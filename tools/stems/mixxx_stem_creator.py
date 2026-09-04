#!/usr/bin/env python3
# ==============================================================================
# DISCLAIMER: The following code and comments were autonomously generated
# by an AI Agent in accordance with the Mixxx Project AI Policy.
# ==============================================================================
"""
Mixxx Stem Creator
Separates audio tracks into Vocals/Instrumental (2 stems) or 4 stems (Vocals,
Drums, Bass, Other) using Demucs, then packages the resulting audio streams
into an official Native Instruments STEM (.stem.mp4) container readable by Mixxx.
"""

import argparse
import json
import os
import shutil
import site
import struct
import subprocess
import sys
import tempfile

# Ensure user site-packages (e.g. where pip install --user installs packages like demucs and numpy)
# are explicitly loaded in sys.path
try:
    user_site = site.getusersitepackages()
    if user_site and os.path.isdir(user_site) and user_site not in sys.path:
        sys.path.insert(0, user_site)
except Exception:
    pass

for ver in ["Python311", "Python312", "Python310", "Python313", "Python39"]:
    candidate = os.path.expandvars(rf"%APPDATA%\Python\{ver}\site-packages")
    if os.path.isdir(candidate) and candidate not in sys.path:
        sys.path.insert(0, candidate)

# Update os.environ['PYTHONPATH'] so child subprocesses also inherit the paths
extra_dirs = [p for p in sys.path if p and os.path.isdir(p)]
if extra_dirs:
    existing_py_path = os.environ.get("PYTHONPATH", "")
    os.environ["PYTHONPATH"] = os.pathsep.join(extra_dirs) + (os.pathsep + existing_py_path if existing_py_path else "")



def log_progress(percent: int, message: str):
    """Prints a structured progress line for Mixxx to parse."""
    print(f"PROGRESS:{percent}:{message}", flush=True)


def find_ffmpeg(custom_path: str = None) -> str:
    """Finds the ffmpeg executable."""
    if custom_path and os.path.isfile(custom_path):
        return custom_path
    path = shutil.which("ffmpeg")
    if path:
        return path
    raise FileNotFoundError("ffmpeg not found in PATH or custom path.")


def inject_stem_atom(mp4_path: str, stem_metadata_json: str):
    """
    Injects the 'stem' atom containing the JSON manifest into moov -> udta -> stem.
    Mixxx's StemInfoImporter specifically looks for path ['moov', 'udta', 'stem'].
    """
    with open(mp4_path, "rb") as f:
        data = bytearray(f.read())

    # Build the stem box: [size: 4B BE][type: 'stem'][json bytes]
    json_bytes = stem_metadata_json.encode("utf-8")
    stem_box_size = 8 + len(json_bytes)
    stem_box = struct.pack(">I4s", stem_box_size, b"stem") + json_bytes

    # Parse top-level boxes to locate 'moov'
    offset = 0
    moov_offset = -1
    moov_size = -1

    while offset < len(data):
        if offset + 8 > len(data):
            break
        box_size, box_type = struct.unpack_from(">I4s", data, offset)
        if box_size == 1:
            box_size = struct.unpack_from(">Q", data, offset + 8)[0]
        elif box_size == 0:
            box_size = len(data) - offset

        if box_type == b"moov":
            moov_offset = offset
            moov_size = box_size
            break
        offset += box_size

    if moov_offset == -1:
        raise ValueError("Invalid MP4 file: 'moov' atom not found.")

    # Search for 'udta' inside 'moov'
    moov_data_start = moov_offset + 8
    moov_data_end = moov_offset + moov_size
    curr = moov_data_start
    udta_offset = -1
    udta_size = -1

    while curr < moov_data_end:
        if curr + 8 > moov_data_end:
            break
        b_size, b_type = struct.unpack_from(">I4s", data, curr)
        if b_size == 1:
            b_size = struct.unpack_from(">Q", data, curr + 8)[0]
        elif b_size == 0:
            b_size = moov_data_end - curr

        if b_type == b"udta":
            udta_offset = curr
            udta_size = b_size
            break
        curr += b_size

    if udta_offset != -1:
        # udta exists: append stem_box inside udta
        insert_pos = udta_offset + udta_size
        data[insert_pos:insert_pos] = stem_box

        # Update udta box size
        new_udta_size = udta_size + stem_box_size
        struct.pack_into(">I", data, udta_offset, new_udta_size)

        # Update moov box size
        new_moov_size = moov_size + stem_box_size
        struct.pack_into(">I", data, moov_offset, new_moov_size)
    else:
        # udta does not exist: create udta box wrapping stem_box
        udta_box_size = 8 + stem_box_size
        udta_box = struct.pack(">I4s", udta_box_size, b"udta") + stem_box

        # Insert at the end of moov
        insert_pos = moov_offset + moov_size
        data[insert_pos:insert_pos] = udta_box

        # Update moov box size
        new_moov_size = moov_size + udta_box_size
        struct.pack_into(">I", data, moov_offset, new_moov_size)

    with open(mp4_path, "wb") as f:
        f.write(data)


def create_stem_file(input_file: str,
                     output_file: str,
                     mode: str = "2stems",
                     model: str = "htdemucs",
                     device: str = "auto",
                     ffmpeg_path: str = None):
    """
    Main pipeline:
    1. Separate audio with Demucs
    2. Combine into 5-stream MP4 via ffmpeg
    3. Inject NI Stem manifest JSON into MP4 container
    """
    log_progress(5, "Verifying audio file and dependencies...")
    ffmpeg_bin = find_ffmpeg(ffmpeg_path)

    if not os.path.isfile(input_file):
        raise FileNotFoundError(f"Input file not found: {input_file}")

    temp_dir = tempfile.mkdtemp(prefix="mixxx_stem_")
    try:
        log_progress(10, f"Running Demucs model ({model}) on {device}...")

        # Build demucs separation command
        cmd = [
            sys.executable, "-m", "demucs.separate",
            "-n", model,
            "-o", temp_dir,
        ]
        if device != "auto":
            cmd.extend(["-d", device])

        if mode == "2stems":
            cmd.append("--two-stems=vocals")

        cmd.append(input_file)

        sub_env = os.environ.copy()
        sub_env.pop("PYTHONNOUSERSITE", None)
        sub_env["PYTHONPATH"] = os.pathsep.join([p for p in sys.path if p])

        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1,
            env=sub_env
        )

        demucs_output_lines = []
        for line in proc.stdout:
            line_str = line.strip()
            demucs_output_lines.append(line_str)
            # Parse demucs progress if available, e.g. " 45%|..."
            if "%" in line_str:
                try:
                    pct_str = line_str.split("%")[0].split()[-1]
                    pct = int(float(pct_str))
                    # Map demucs 0-100% to progress 15-75%
                    mapped = 15 + int(pct * 0.60)
                    log_progress(mapped, f"Separating audio with Demucs: {pct}%")
                except Exception:
                    pass
            elif line_str:
                print(f"[Demucs] {line_str}", flush=True)

        proc.wait()
        if proc.returncode != 0:
            error_details = "\n".join(demucs_output_lines[-10:]) if demucs_output_lines else "No output from Demucs."
            raise RuntimeError(f"Demucs process failed with exit code {proc.returncode}:\n{error_details}")

        log_progress(75, "Audio separation completed. Preparing stems...")

        # Find separated output directory inside temp_dir
        base_name = os.path.splitext(os.path.basename(input_file))[0]
        model_out_dir = os.path.join(temp_dir, model, base_name)
        if not os.path.isdir(model_out_dir):
            # Try searching for directory in temp_dir
            for root, dirs, files in os.walk(temp_dir):
                if "vocals.wav" in files:
                    model_out_dir = root
                    break

        if not os.path.isdir(model_out_dir):
            raise FileNotFoundError("Demucs output files could not be found.")

        # Mixxx expects 5 stereo streams in this exact layout:
        # Stream 0: Master (original)
        # Stream 1: Drums
        # Stream 2: Bass
        # Stream 3: Other
        # Stream 4: Vocals
        # (Standard NI colors: Drums=#009E73, Bass=#D55E00, Other=#CC79A7, Vocals=#56B4E9)
        vocals_wav = os.path.join(model_out_dir, "vocals.wav")

        if mode == "2stems":
            # Demucs --two-stems=vocals creates: vocals.wav and no_vocals.wav
            no_vocals_wav = os.path.join(model_out_dir, "no_vocals.wav")
            # For 2 stems, we map:
            # Stem 1: Instrumental (no_vocals)
            # Stem 2: Instrumental
            # Stem 3: Instrumental
            # Stem 4: Vocals
            stem1_file = no_vocals_wav
            stem2_file = no_vocals_wav
            stem3_file = no_vocals_wav
            stem4_file = vocals_wav

            manifest = {
                "version": 1,
                "stems": [
                    {"name": "Instrumental", "color": "#009E73"},
                    {"name": "Instrumental", "color": "#D55E00"},
                    {"name": "Instrumental", "color": "#CC79A7"},
                    {"name": "Vocals", "color": "#56B4E9"}
                ]
            }
        else:
            # 4 stems: drums, bass, other, vocals
            stem1_file = os.path.join(model_out_dir, "drums.wav")
            stem2_file = os.path.join(model_out_dir, "bass.wav")
            stem3_file = os.path.join(model_out_dir, "other.wav")
            stem4_file = vocals_wav

            manifest = {
                "version": 1,
                "stems": [
                    {"name": "Drums", "color": "#009E73"},
                    {"name": "Bass", "color": "#D55E00"},
                    {"name": "Other", "color": "#CC79A7"},
                    {"name": "Vocals", "color": "#56B4E9"}
                ]
            }

        log_progress(80, "Encoding 5-stream MP4 container via ffmpeg...")
        os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
        temp_mp4 = os.path.join(temp_dir, "temp_stems.mp4")

        # Encode with AAC 256k per stream. Faststart puts moov at beginning.
        ffmpeg_cmd = [
            ffmpeg_bin, "-y",
            "-i", input_file,
            "-i", stem1_file,
            "-i", stem2_file,
            "-i", stem3_file,
            "-i", stem4_file,
            "-map", "0:a",
            "-map", "1:a",
            "-map", "2:a",
            "-map", "3:a",
            "-map", "4:a",
            "-c:a", "aac",
            "-b:a", "256k",
            "-ac", "2",
            "-ar", "44100",
            temp_mp4
        ]

        res = subprocess.run(ffmpeg_cmd, capture_output=True, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"FFmpeg encoding failed: {res.stderr}")

        log_progress(92, "Injecting Native Instruments STEM metadata...")
        inject_stem_atom(temp_mp4, json.dumps(manifest))

        log_progress(97, "Finalizing .stem.mp4 file...")
        shutil.move(temp_mp4, output_file)

        log_progress(100, f"Done! Created stem file: {output_file}")

    finally:
        shutil.rmtree(temp_dir, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description="Mixxx Stem Creator (Demucs to .stem.mp4)")
    parser.add_argument("--input", "-i", required=True, help="Path to input audio track")
    parser.add_argument("--output", "-o", required=True, help="Path to output .stem.mp4 track")
    parser.add_argument("--mode", "-m", choices=["2stems", "4stems"], default="2stems",
                        help="Separation mode (2stems: Vocals/Inst, 4stems: Vocals/Drums/Bass/Other)")
    parser.add_argument("--model", default="htdemucs", help="Demucs model name (default: htdemucs)")
    parser.add_argument("--device", "-d", default="auto", choices=["auto", "cuda", "cpu"],
                        help="Processing device (auto, cuda, cpu)")
    parser.add_argument("--ffmpeg", help="Custom path to ffmpeg binary")

    args = parser.parse_args()

    try:
        create_stem_file(
            input_file=args.input,
            output_file=args.output,
            mode=args.mode,
            model=args.model,
            device=args.device,
            ffmpeg_path=args.ffmpeg
        )
    except Exception as e:
        print(f"ERROR: {str(e)}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

# ==============================================================================
# DISCLAIMER: End of autonomously generated code by AI Agent.
# ==============================================================================
