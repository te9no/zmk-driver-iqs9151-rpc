# IQS9151 Studio Web UI

DYA Studio / ZMK Studio Custom RPC UI for the zmk__iqs9151 subsystem.

## Requirements

Run:

    npm install

## Development

Run:

    npm run dev

The firmware advertises http://localhost:5173 while developing locally.

## Production Build

Run:

    npm run build

npm run build uses the generated TypeScript files under src/proto.
Run npm run generate only after changing ../proto/zmk/iqs9151/iqs9151.proto.

    npm run generate

## Demo Mode

Use Enable Demo in the Web UI to test the screen without a connected keyboard.
Demo mode does not call RPC; it only updates local browser state.
