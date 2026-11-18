/* Decode HbbTV QOI pixels off the Live-TV interaction thread.
 * The ordinary hbbtv-qoi.js decoder remains the single codec implementation. */
'use strict';
self.window = self;
importScripts('hbbtv-qoi.js');
self.onmessage = function(event) {
  const message = event && event.data ? event.data : {};
  const id = message.id;
  try {
    const decoded = self.VdrSuiteQoi.decode(message.bytes);
    self.postMessage({
      id: id,
      width: decoded.width,
      height: decoded.height,
      channels: decoded.channels,
      colorspace: decoded.colorspace,
      pixels: decoded.pixels
    }, [decoded.pixels.buffer]);
  } catch (error) {
    self.postMessage({
      id: id,
      error: error && error.message ? error.message : 'hbbtv_qoi_decode_failed'
    });
  }
};
