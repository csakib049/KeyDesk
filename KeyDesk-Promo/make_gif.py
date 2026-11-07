from pathlib import Path

from PIL import Image


root = Path(__file__).resolve().parent
frames_dir = root / "renders" / "keydesk-gif-frames"
output = root.parent / "Images" / "KeyDesk-demo.gif"
paths = sorted(frames_dir.glob("frame-*.png"))
if not paths:
    raise SystemExit(f"No PNG frames found in {frames_dir}")

images = [Image.open(path).convert("RGB") for path in paths]
thumb_w, thumb_h = 240, 135
columns = 10
rows = (len(images) + columns - 1) // columns
sheet = Image.new("RGB", (columns * thumb_w, rows * thumb_h), "#0e0b16")
for index, image in enumerate(images):
    thumb = image.resize((thumb_w, thumb_h), Image.Resampling.LANCZOS)
    sheet.paste(thumb, ((index % columns) * thumb_w, (index // columns) * thumb_h))

palette = sheet.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
gif_frames = [image.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG) for image in images]
output.parent.mkdir(parents=True, exist_ok=True)
gif_frames[0].save(
    output,
    save_all=True,
    append_images=gif_frames[1:],
    duration=80,
    loop=0,
    optimize=True,
    disposal=2,
)
print(f"Wrote {output} ({len(gif_frames)} frames, 8 seconds, looping forever)")
