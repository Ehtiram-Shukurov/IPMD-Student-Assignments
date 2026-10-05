# Initial Upload — Ehtiram Shukurov

**Project:** EchoSphere video-to-music pipeline (my part of the EchoSphere robot project).
**Note:** This is the initial project upload — weekly updates will follow in `week2/`, `week3/`, etc.
**Repo:** [Ehtiram-Shukurov/echosphere-music](https://github.com/Ehtiram-Shukurov/echosphere-music)
**Live demo:** https://ehtiram-shukurov.github.io/echosphere-music/

## What it does

Upload a video of the EchoSphere robot → it finds the glowing sphere, reads the mood from its light (Warm / Calm / Sad / Anger) → picks and plays a matching licensed track. All in the browser, no server.

## What I built this week

- **Energy-matched track picking** — songs are scored by how close their feel is to the mood's ideal point; the seed picks among the 3 closest. Same code in Python and JavaScript, verified by parity tests.
- **Beat-snapped excerpts** — every track's beat positions are pre-measured into the manifest; excerpts start on a beat instead of mid-phrase.
- **Mood-following playlist** — the timeline splits into mood stretches; each gets its own energy-matched track, crossfaded together.
- **Export video** — records the video with its soundtrack as a downloadable MP4: clean picture (no detection outlines), up to 1080p, seekable.
- **Scene fallback** — if the sphere can't be found, the whole scene is read, but only when its colors clearly point to one mood.
- **Docs** — brought README, guide, and browser docs up to date.

## What's in this folder

`echosphere-music/` is a snapshot of the project (main branch). Two files were too large for the upload tool and are excluded, but live in the source repo:
- `music-page.html` (7.2 MB standalone composer page — not part of this pipeline)
- `tests/js/fixtures/palette-cases.json` (394 KB test fixture)

The `music-library/*.mp3` files (475 MB) are also excluded; `music-library/manifest.json` (included) describes every track.
