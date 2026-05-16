# IQS9151 Studio Web UI

DYA Studio / ZMK Studio Custom RPC UI for the dya__iqs9151 subsystem.

## Requirements

Run:

    npm install

## Development

Run:

    npm run dev

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
