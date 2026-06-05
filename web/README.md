# IQS9151 Studio Web UI

DYA Studio / ZMK Studio Custom RPC UI for the dya__iqs9151 subsystem.

## Requirements

Run:

    npm ci

## Development

Run:

    npm run dev -- --host 127.0.0.1 --port 5173

Production firmware advertises https://te9no.github.io/zmk-driver-iqs9151-rpc/.
For local development, run npm run dev and temporarily advertise http://localhost:5173.

## Production Build

Run:

    npm run build

npm run build uses the generated TypeScript files under src/proto.
Run npm run generate only after changing ../proto/zmk/iqs9151/iqs9151.proto.

    npm run generate

## Demo Mode

Use Enable Demo in the Web UI to test the screen without a connected keyboard.
Demo mode does not call RPC; it only updates local browser state.

## Runtime Behavior

- The UI searches for `dya__iqs9151`, `zmk__iqs9151`, then `iqs9151`.
- Load, apply, and reset operations are serialized so repeated clicks do not
  start overlapping RPC writes.
- RPC calls use a bounded timeout. If a split peripheral or custom subsystem is
  not responding, the UI returns to an operable state and shows the error.
- Text edits stay local until `Apply` is pressed. Demo mode never writes to
  firmware.

## Deployment

GitHub Pages deployment is handled by `.github/workflows/web-ui-pages.yml`.
The workflow builds `web/` with Node.js 24 and publishes `web/dist`.
