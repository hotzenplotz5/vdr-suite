#!/usr/bin/env python3
"""Read-only offline diagnosis of a VDR HEVC MPEG-TS recording and fMP4 remux.

No TV, daemon, production directory or recording is modified.
Run as an unprivileged user. Requires ffmpeg/ffprobe and Python 3.9+.
"""
import argparse
import collections
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys
import tempfile

DEFAULT_SOURCE = Path('/srv/vdr/video/Historienfilm/Der_Medicus_II/2026-05-18.19.00.1-0.rec/00001.ts')


def run(argv, *, cwd=None, timeout=210):
    try:
        p = subprocess.run(argv, cwd=cwd, capture_output=True, text=True,
                           encoding='utf-8', errors='replace', timeout=timeout,
                           check=False, stdin=subprocess.DEVNULL)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired:
        return 124, '', 'TIMEOUT'
    except OSError as exc:
        return 125, '', type(exc).__name__ + ': ' + str(exc)


def probe_json(argv, *, cwd=None):
    rc, out, err = run(['ffprobe', '-v', 'error', *argv, '-of', 'json'], cwd=cwd)
    if rc:
        print('  FFPROBE_ERROR:', err.strip()[:320], 'exit=' + str(rc))
        return None
    try:
        return json.loads(out)
    except ValueError:
        print('  FFPROBE_JSON_INVALID')
        return None


def num(value):
    try:
        x = float(value)
        return x if x == x and abs(x) != float('inf') else None
    except (TypeError, ValueError):
        return None


def percentile(vals, rank):
    vals = sorted(vals)
    return vals[int((len(vals) - 1) * rank)] if vals else None


def describe_packets(packets, title):
    # Decode timestamps are compared in packet/decoding order; presentation
    # timestamps are sorted because B-frames naturally reorder PTS.
    pts = [num(p.get('pts_time')) for p in packets]
    dts = [num(p.get('dts_time')) for p in packets]
    missing_pts = sum(p is None for p in pts)
    missing_dts = sum(d is None for d in dts)
    dts_pairs = [(a, b) for a, b in zip(dts, dts[1:])
                 if a is not None and b is not None]
    dts_bad = sum(b <= a + 1e-8 for a, b in dts_pairs)
    presentation = sorted(p for p in pts if p is not None)
    gaps = [b - a for a, b in zip(presentation, presentation[1:])]
    tiny = sum(x <= 0.001 for x in gaps)
    large = sum(x >= 0.075 for x in gaps)
    common = collections.Counter(round(x * 1000, 1) for x in gaps if x > 0)
    positive_durations = [num(p.get('duration_time')) for p in packets]
    durations_bad = sum(v is not None and v <= 0 for v in positive_durations)
    keyframes = sum('K' in str(p.get('flags', '')) for p in packets)
    print(f'{title}: packets={len(packets)} keyframes={keyframes} '
          f'pts_missing={missing_pts} dts_missing={missing_dts} '
          f'dts_non_increasing={dts_bad} pts_delta_le_1ms={tiny} '
          f'pts_gaps_ge_75ms={large} duration_nonpositive={durations_bad}')
    if presentation:
        print(f'  pts_first={presentation[0]:.4f}s pts_last={presentation[-1]:.4f}s '
              f'pts_span={presentation[-1] - presentation[0]:.3f}s')
    if gaps:
        print('  pts_delta_ms: p05={:.3f} median={:.3f} p95={:.3f} '
              'frequent={}'.format(percentile(gaps, .05)*1000,
                                   statistics.median(gaps)*1000,
                                   percentile(gaps, .95)*1000,
                                   common.most_common(5)))
    return {'pts': presentation, 'dts_bad': dts_bad, 'tiny': tiny,
            'large': large, 'packets': len(packets)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=DEFAULT_SOURCE,
                        help='Only read this recording TS file')
    parser.add_argument('--seconds', type=int, default=24,
                        help='Analysis time (8-60 s, default 24)')
    args = parser.parse_args()
    if not (8 <= args.seconds <= 60):
        parser.error('--seconds must be between 8 and 60')
    if os.geteuid() == 0:
        print('STOP_ROOT: Bitte als Benutzer yavdr ausfuehren, nicht als root.')
        return 2
    if not shutil.which('ffmpeg') or not shutil.which('ffprobe'):
        print('STOP_DEPENDENCY: ffmpeg und ffprobe fehlen.')
        return 2
    src = args.source
    if not src.is_file() or not os.access(src, os.R_OK):
        print('STOP_SOURCE: Datei nicht lesbar:', src)
        return 2
    try:
        source_resolved = src.resolve(strict=True)
    except OSError as exc:
        print('STOP_SOURCE:', exc)
        return 2
    scratch = Path(tempfile.gettempdir())
    usage = shutil.disk_usage(scratch)
    reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
    budget = 2 * 1024**3
    print('RESOURCE_PREFLIGHT total={} free={} reserve={} budget={}'.format(
        usage.total, usage.free, reserve, budget), flush=True)
    if usage.free < reserve + budget:
        print('STOP_DISK: temporaeres Dateisystem hat zu wenig Reserve.')
        return 2
    print('RESOURCE_PREFLIGHT=PASS', flush=True)
    print('=== HEVC-OFFLINE-ANALYSE V1 ===', flush=True)
    print('SOURCE:', source_resolved)
    print('SEKUNDEN:', args.seconds, 'READ_ONLY_SOURCE=YES TV_REQUIRED=NO')
    print('FFMPEG:', run(['ffmpeg', '-version'], timeout=10)[1].splitlines()[0])

    print('\n=== ORIGINAL: CODEC ===', flush=True)
    meta = probe_json(['-select_streams', 'v:0', '-show_streams',
                       '-show_entries', 'stream=codec_name,profile,level,width,height,'
                       'pix_fmt,avg_frame_rate,r_frame_rate,time_base,has_b_frames',
                       str(src)])
    if meta:
        stream = (meta.get('streams') or [{}])[0]
        keys = ('codec_name','profile','level','width','height','pix_fmt',
                'avg_frame_rate','r_frame_rate','time_base','has_b_frames')
        print(json.dumps({k: stream[k] for k in keys if k in stream},
                         ensure_ascii=False, sort_keys=True))

    print('\n=== ORIGINAL: PTS/DTS ===', flush=True)
    source_packet_data = probe_json([
        '-read_intervals', f'%+{args.seconds}', '-select_streams', 'v:0',
        '-show_packets', '-show_entries',
        'packet=pts_time,dts_time,duration_time,flags', str(src)])
    original = describe_packets(source_packet_data.get('packets', []), 'ORIGINAL') \
        if source_packet_data is not None else None

    print('\n=== ORIGINAL: DEKODIERTE BILDER ===', flush=True)
    frame_data = probe_json([
        '-read_intervals', f'%+{args.seconds}', '-select_streams', 'v:0',
        '-show_frames', '-show_entries',
        'frame=best_effort_timestamp_time,pict_type,key_frame', str(src)])
    frames = (frame_data or {}).get('frames', [])
    if frame_data is not None:
        frame_times = sorted(v for v in
                             (num(f.get('best_effort_timestamp_time')) for f in frames)
                             if v is not None)
        diffs = [b-a for a,b in zip(frame_times, frame_times[1:])]
        print('decoded_frames=', len(frames), 'presentation_pts_known=', len(frame_times),
              'pict_types=', dict(collections.Counter(f.get('pict_type') for f in frames)))
        if diffs:
            print('  frame_pts_delta_ms: median={:.3f} p95={:.3f} '
                  'gaps_ge_75ms={}'.format(statistics.median(diffs)*1000,
                                         percentile(diffs, .95)*1000,
                                         sum(d >= .075 for d in diffs)))

    print('\n=== ORIGINAL: INHALTLICHE FRAME-DUPLIKATE ===', flush=True)
    cmd = ['ffmpeg', '-hide_banner', '-nostdin', '-loglevel', 'info',
           '-threads', '2', '-i', str(src), '-t', str(args.seconds),
           '-map', '0:v:0', '-an', '-sn', '-vf', 'showinfo,mpdecimate,showinfo',
           '-f', 'null', '-']
    rc, _, err = run(cmd, timeout=210)
    showinfo = collections.defaultdict(list)
    for filt, frame_n in re.findall(
            r'\[Parsed_showinfo_(\d+)[^\]]*\]\s+n:\s*(\d+)', err):
        showinfo[int(filt)].append(int(frame_n))
    stages = sorted(showinfo)
    decoded_count = max(showinfo[stages[0]]) + 1 if stages else None
    kept_count = max(showinfo[stages[-1]]) + 1 if len(stages) > 1 else None
    if rc:
        print('MPDECIMATE_ERROR: rc=', rc, err[-350:])
    else:
        print('mpdecimate_retained_frames=', kept_count,
              'decoded_frames_same_pass=', decoded_count)
        if kept_count is not None and decoded_count is not None:
            print('mpdecimate_potential_near_duplicates=',
                  max(0, decoded_count-kept_count),
                  '(nur Heuristik; ruhige Szenen koennen betroffen sein)')

    print('\n=== A/B-REMUX: VIDEO-COPY → HLS-fMP4 ===', flush=True)
    # TemporaryDirectory removes only its own, newly created private folder;
    # its symlink to the source is unlinked, never followed.
    with tempfile.TemporaryDirectory(prefix='vdr-hevc-offline-') as temp_name:
        work = Path(temp_name)
        for tag, flags in (
            ('BASELINE_SPLIT', 'delete_segments+split_by_time+temp_file'),
            ('TRIAL_KEYFRAME', 'delete_segments+independent_segments+temp_file'),
        ):
            td = work / tag
            td.mkdir(mode=0o700)
            (td / 'source-000001.ts').symlink_to(source_resolved)
            (td / 'input.ffconcat').write_text(
                "ffconcat version 1.0\nfile 'source-000001.ts'\n", encoding='ascii')
            cmd = ['ffmpeg', '-hide_banner', '-nostdin', '-loglevel', 'warning', '-n',
                   '-f', 'concat', '-safe', '1', '-i', 'input.ffconcat',
                   '-t', str(args.seconds), '-map', '0:v:0?', '-map', '0:a:0?',
                   '-sn', '-c:v', 'copy', '-c:a', 'aac', '-b:a', '192k', '-ac', '2',
                   '-f', 'hls', '-hls_time', '4', '-hls_list_size', '0',
                   '-hls_flags', flags,
                   '-hls_segment_type', 'fmp4', '-hls_fmp4_init_filename', 'init.mp4',
                   '-hls_segment_filename', 'segment-%06d.m4s', 'master.m3u8']
            print('\nREMUX_VARIANT=', tag, 'FLAGS=', flags, flush=True)
            print('REMUX_NOTE: -re ist fuer den Offline-Test weggelassen; '
                  'alle erzeugten Segmente bleiben bis zur ffprobe-Auswertung erhalten.')
            rc, _, fferr = run(cmd, cwd=td, timeout=180)
            dts_warn = [line for line in fferr.splitlines()
                        if 'Non-monotonic DTS' in line]
            dur_warn = [line for line in fferr.splitlines()
                        if 'Packet duration:' in line and 'out of range' in line]
            print('remux_rc=', rc, 'nonmonotonic_dts_warnings=', len(dts_warn),
                  'mp4_out_of_range_duration_warnings=', len(dur_warn))
            for sample in (dts_warn[:2] + dur_warn[:2]):
                print('  EXAMPLE:', sample[:260])
            if rc:
                print('  REMUX_ERROR:', fferr[-700:])
            if (td / 'master.m3u8').exists():
                playlist = (td / 'master.m3u8').read_text(errors='replace')
                durations = [float(x) for x in re.findall(r'#EXTINF:([\d.]+)', playlist)]
                print('HLS_SEGMENTS=', len(durations), 'duration_seconds=',
                      [round(x, 3) for x in durations[:15]],
                      'ENDLIST=', '#EXT-X-ENDLIST' in playlist)
                # Inspect exactly the initialization box followed by the
                # fMP4 fragments, as passed to MSE, rather than relying on
                # the ffprobe HLS playlist demuxer to reconstruct timestamps.
                segments = sorted(td.glob('segment-*.m4s'))
                init = td / 'init.mp4'
                if not init.is_file() or len(segments) != len(durations):
                    print('  FMP4_INCOMPLETE: init or segment missing')
                    continue
                joined = td / 'mse-appended-stream.mp4'
                with joined.open('wb') as target:
                    for part in [init, *segments]:
                        with part.open('rb') as inp:
                            shutil.copyfileobj(inp, target, length=1024 * 1024)
                out_meta = probe_json(['-select_streams', 'v:0', '-show_entries',
                                       'stream=codec_name,codec_tag_string,profile,'
                                       'level,avg_frame_rate,r_frame_rate,time_base',
                                       '-show_streams', str(joined)], cwd=td)
                if out_meta:
                    track = (out_meta.get('streams') or [{}])[0]
                    keys = ('codec_name', 'codec_tag_string', 'profile',
                            'level', 'avg_frame_rate', 'r_frame_rate', 'time_base')
                    print('  FMP4_CODEC:',
                          {k:track[k] for k in keys if k in track})
                out_data = probe_json(['-select_streams', 'v:0', '-show_packets',
                                       '-show_entries',
                                       'packet=pts_time,dts_time,duration_time,flags',
                                       str(joined)], cwd=td)
                output = describe_packets(out_data.get('packets', []), tag + '_FMP4') \
                    if out_data is not None else None
                if original and output:
                    print('FRAME_PACKET_COUNT_DELTA=', output['packets']-original['packets'],
                          ' (bei -t und Zeitbasis koennen Randeffekte auftreten)')
                    for label, data in (('ORIGINAL', original), (tag, output)):
                        pts = data['pts']
                        if len(pts) >= 2:
                            span = pts[-1] - pts[0]
                            print(f'{label}_PTS_AVG_RATE={((len(pts)-1)/span if span>0 else 0):.4f}/s')

    print('\n=== AUSWERTUNG ===')
    print('Hohe Zahl near-duplicates im Original: Quelldatei/Bildinhalt untersuchen.')
    print('Neue/mehr PTS-Luecken oder MP4-Warnungen: Remux-Zeitbasis untersuchen.')
    print('Gleichmaessige 24 fps in Original UND fMP4: VIDAA-Ausgabe bleibt moeglich.')
    print('Kein Offline-Test kann reale Display-Cadence des TV nachweisen.')
    print('=== HEVC_REPORT_END ===', flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
