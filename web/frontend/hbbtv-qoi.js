(function(global) {
  'use strict';

  if (global.VdrSuiteQoi) return;

  const HEADER_BYTES = 14;
  const END_BYTES = 8;
  const MAX_WIDTH = 3840;
  const MAX_HEIGHT = 2160;
  const MAX_PIXELS = MAX_WIDTH * MAX_HEIGHT;

  function asBytes(input) {
    if (input instanceof Uint8Array) return input;
    if (input instanceof ArrayBuffer) return new Uint8Array(input);
    if (ArrayBuffer.isView(input)) {
      return new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
    }
    throw new Error('hbbtv_qoi_input_invalid');
  }

  function read32(bytes, offset) {
    return (
      (bytes[offset] * 0x1000000) +
      (bytes[offset + 1] << 16) +
      (bytes[offset + 2] << 8) +
      bytes[offset + 3]
    ) >>> 0;
  }

  function hash(r, g, b, a) {
    return (r * 3 + g * 5 + b * 7 + a * 11) & 63;
  }

  function decode(input) {
    const bytes = asBytes(input);
    if (bytes.length < HEADER_BYTES + END_BYTES) {
      throw new Error('hbbtv_qoi_truncated');
    }
    if (
      bytes[0] !== 0x71 ||
      bytes[1] !== 0x6f ||
      bytes[2] !== 0x69 ||
      bytes[3] !== 0x66
    ) {
      throw new Error('hbbtv_qoi_magic_invalid');
    }

    const width = read32(bytes, 4);
    const height = read32(bytes, 8);
    const channels = bytes[12];
    if (
      width < 1 || height < 1 ||
      width > MAX_WIDTH || height > MAX_HEIGHT ||
      (channels !== 3 && channels !== 4)
    ) {
      throw new Error('hbbtv_qoi_dimensions_invalid');
    }

    const pixelCount = width * height;
    if (!Number.isSafeInteger(pixelCount) || pixelCount > MAX_PIXELS) {
      throw new Error('hbbtv_qoi_dimensions_invalid');
    }

    const endOffset = bytes.length - END_BYTES;
    if (
      bytes[endOffset] !== 0 || bytes[endOffset + 1] !== 0 ||
      bytes[endOffset + 2] !== 0 || bytes[endOffset + 3] !== 0 ||
      bytes[endOffset + 4] !== 0 || bytes[endOffset + 5] !== 0 ||
      bytes[endOffset + 6] !== 0 || bytes[endOffset + 7] !== 1
    ) {
      throw new Error('hbbtv_qoi_end_marker_invalid');
    }

    const output = new Uint8ClampedArray(pixelCount * 4);
    const index = new Uint8Array(64 * 4);

    let r = 0;
    let g = 0;
    let b = 0;
    let a = 255;
    let run = 0;
    let source = HEADER_BYTES;

    for (let pixel = 0; pixel < pixelCount; pixel += 1) {
      if (run > 0) {
        run -= 1;
      } else {
        if (source >= endOffset) {
          throw new Error('hbbtv_qoi_pixel_data_truncated');
        }

        const first = bytes[source++];
        if (first === 0xfe) {
          if (source + 2 >= endOffset) {
            throw new Error('hbbtv_qoi_rgb_truncated');
          }
          r = bytes[source++];
          g = bytes[source++];
          b = bytes[source++];
        } else if (first === 0xff) {
          if (source + 3 >= endOffset) {
            throw new Error('hbbtv_qoi_rgba_truncated');
          }
          r = bytes[source++];
          g = bytes[source++];
          b = bytes[source++];
          a = bytes[source++];
        } else {
          const tag = first & 0xc0;
          if (tag === 0x00) {
            const entry = (first & 0x3f) * 4;
            r = index[entry];
            g = index[entry + 1];
            b = index[entry + 2];
            a = index[entry + 3];
          } else if (tag === 0x40) {
            r = (r + (((first >> 4) & 0x03) - 2)) & 0xff;
            g = (g + (((first >> 2) & 0x03) - 2)) & 0xff;
            b = (b + ((first & 0x03) - 2)) & 0xff;
          } else if (tag === 0x80) {
            if (source >= endOffset) {
              throw new Error('hbbtv_qoi_luma_truncated');
            }
            const second = bytes[source++];
            const dg = (first & 0x3f) - 32;
            r = (r + dg + ((second >> 4) & 0x0f) - 8) & 0xff;
            g = (g + dg) & 0xff;
            b = (b + dg + (second & 0x0f) - 8) & 0xff;
          } else {
            run = first & 0x3f;
          }
        }
      }

      const slot = hash(r, g, b, a) * 4;
      index[slot] = r;
      index[slot + 1] = g;
      index[slot + 2] = b;
      index[slot + 3] = a;

      const target = pixel * 4;
      output[target] = r;
      output[target + 1] = g;
      output[target + 2] = b;
      output[target + 3] = a;
    }

    if (source !== endOffset) {
      throw new Error('hbbtv_qoi_trailing_pixel_data');
    }

    return {
      width: width,
      height: height,
      channels: channels,
      colorspace: bytes[13],
      pixels: output
    };
  }

  global.VdrSuiteQoi = Object.freeze({
    decode: decode
  });
})(window);
