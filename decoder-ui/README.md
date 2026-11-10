# EXEED CAN Decoder Lab UI

Standalone Vite wrapper for the read-only CAN Decoder Lab.

## Development

```bash
npm install
npm run dev
```

## Production build

```bash
npm run build
```

Vercel settings:
- Root Directory: `decoder-ui`
- Framework Preset: Vite
- Build Command: `npm run build`
- Output Directory: `dist`

The page connects to the same-origin `/viewer` WebSocket endpoint when hosted by the CAN Cloud worker. For a separate Vercel deployment, configure the Cloudflare endpoint through the UI's live connection setting in a later integration step. The interface is read-only and never sends CAN frames.
