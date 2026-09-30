"""Prepare MiniTV MJPEG and MP3 channel folders on a desktop computer."""

from __future__ import annotations

import os
import queue
import shutil
import subprocess
import tempfile
import threading
import tkinter as tk
from dataclasses import dataclass
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Callable
from urllib.parse import urlsplit


VIDEO_SUFFIXES = {".avi", ".mkv", ".mp4", ".webm", ".mov"}
PROFILES = {
    "AI-PI Lite 128 x 128 (experimental)": (128, 128, 10, 16000),
    "MiniTV 288 x 240": (288, 240, 30, 44100),
    "CYD 320 x 240": (320, 240, 24, 44100),
}


@dataclass(frozen=True)
class ConversionOptions:
    """Hold validated encoder options for one conversion run."""

    width: int
    height: int
    fps: int
    audio_rate: int
    quality: int
    volume_db: float


class MiniTVConverter:
    """Provide a Tkinter interface for creating MiniTV channel directories."""

    def __init__(self, root: tk.Tk) -> None:
        """Initialize the desktop interface and background-work state."""
        self.root = root
        self.root.title("MiniTV Video Converter")
        self.root.minsize(620, 520)
        self.input_dir = tk.StringVar()
        self.output_dir = tk.StringVar()
        self.profile = tk.StringVar(value=next(iter(PROFILES)))
        self.fps = tk.StringVar(value="10")
        self.quality = tk.StringVar(value="8")
        self.volume = tk.StringVar(value="-8")
        self.video_url = tk.StringVar()
        self.events: queue.Queue[tuple[str, str]] = queue.Queue()
        self.stop_event = threading.Event()
        self.worker: threading.Thread | None = None
        self.process_lock = threading.Lock()
        self.process: subprocess.Popen[str] | None = None
        self.ffmpeg_path = "ffmpeg"
        self._build_interface()
        self.root.after(100, self._poll_events)

    def _build_interface(self) -> None:
        """Build labeled controls, status text, and a keyboard-friendly log."""
        frame = ttk.Frame(self.root, padding=12)
        frame.grid(sticky="nsew")
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        frame.columnconfigure(1, weight=1)
        frame.rowconfigure(7, weight=1)

        ttk.Label(frame, text="Source folder").grid(row=0, column=0, sticky="w", padx=4, pady=4)
        ttk.Entry(frame, textvariable=self.input_dir, state="readonly").grid(row=0, column=1, sticky="ew", padx=4, pady=4)
        ttk.Button(frame, text="Browse…", command=self._choose_input).grid(row=0, column=2, padx=4, pady=4)

        ttk.Label(frame, text="Output folder").grid(row=1, column=0, sticky="w", padx=4, pady=4)
        ttk.Entry(frame, textvariable=self.output_dir, state="readonly").grid(row=1, column=1, sticky="ew", padx=4, pady=4)
        ttk.Button(frame, text="Browse…", command=self._choose_output).grid(row=1, column=2, padx=4, pady=4)

        ttk.Label(frame, text="Encoding profile").grid(row=2, column=0, sticky="w", padx=4, pady=4)
        profile_box = ttk.Combobox(frame, textvariable=self.profile, values=list(PROFILES), state="readonly")
        profile_box.grid(row=2, column=1, columnspan=2, sticky="ew", padx=4, pady=4)
        profile_box.bind("<<ComboboxSelected>>", self._profile_changed)

        options = ttk.Frame(frame)
        options.grid(row=3, column=0, columnspan=3, sticky="ew", pady=4)
        for column, (label, variable) in enumerate((("FPS", self.fps), ("JPEG quality (2–31)", self.quality), ("Audio gain (dB)", self.volume))):
            ttk.Label(options, text=label).grid(row=0, column=column * 2, sticky="w", padx=4)
            ttk.Entry(options, textvariable=variable, width=9).grid(row=0, column=column * 2 + 1, sticky="w", padx=4)

        ttk.Label(frame, text="Optional video URL (requires yt-dlp)").grid(row=4, column=0, sticky="w", padx=4, pady=4)
        ttk.Entry(frame, textvariable=self.video_url).grid(row=4, column=1, sticky="ew", padx=4, pady=4)
        ttk.Button(frame, text="Download and convert", command=self._start_url_conversion).grid(row=4, column=2, padx=4, pady=4)

        actions = ttk.Frame(frame)
        actions.grid(row=5, column=0, columnspan=3, pady=8)
        self.convert_button = ttk.Button(actions, text="Convert source folder", command=self._start_folder_conversion)
        self.convert_button.pack(side=tk.LEFT, padx=4)
        self.stop_button = ttk.Button(actions, text="Stop", command=self._request_stop, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=4)

        self.status = tk.StringVar(value="Select source and output folders. Existing channel outputs are skipped.")
        ttk.Label(frame, textvariable=self.status, wraplength=640).grid(row=6, column=0, columnspan=3, sticky="w", padx=4, pady=4)
        ttk.Label(frame, text="Progress log").grid(row=7, column=0, columnspan=3, sticky="sw", padx=4)
        self.log = tk.Text(frame, height=12, wrap="word", state="disabled")
        self.log.grid(row=8, column=0, columnspan=3, sticky="nsew", padx=4, pady=4)
        frame.rowconfigure(8, weight=1)

    def _choose_input(self) -> None:
        """Let the user select a folder containing supported source videos."""
        selected = filedialog.askdirectory(title="Select source video folder")
        if selected:
            self.input_dir.set(selected)

    def _choose_output(self) -> None:
        """Let the user select where generated channel folders are written."""
        selected = filedialog.askdirectory(title="Select output folder")
        if selected:
            self.output_dir.set(selected)

    def _profile_changed(self, _event: object = None) -> None:
        """Set the profile's suggested frame rate when it is selected."""
        _width, _height, fps, _audio_rate = PROFILES[self.profile.get()]
        self.fps.set(str(fps))

    def _validated_options(self) -> ConversionOptions:
        """Validate profile and numeric options before starting a worker."""
        width, height, _default_fps, audio_rate = PROFILES[self.profile.get()]
        fps = int(self.fps.get())
        quality = int(self.quality.get())
        volume = float(self.volume.get())
        if not 1 <= fps <= 60:
            raise ValueError("FPS must be between 1 and 60.")
        if not 2 <= quality <= 31:
            raise ValueError("JPEG quality must be between 2 and 31.")
        if not -30 <= volume <= 12:
            raise ValueError("Audio gain must be between -30 dB and 12 dB.")
        return ConversionOptions(width, height, fps, audio_rate, quality, volume)

    def _start_folder_conversion(self) -> None:
        """Validate selections and launch conversion for supported local files."""
        source = Path(self.input_dir.get())
        if not source.is_dir():
            messagebox.showerror("Source required", "Select an existing source folder first.")
            return
        output = Path(self.output_dir.get())
        self._start_worker(lambda options: self._convert_folder(source, output, options))

    def _start_url_conversion(self) -> None:
        """Download one URL with an installed yt-dlp executable, then convert it."""
        url = self.video_url.get().strip()
        output = Path(self.output_dir.get())
        if not url:
            messagebox.showerror("URL required", "Enter a video URL first.")
            return
        parsed_url = urlsplit(url)
        if parsed_url.scheme not in {"http", "https"} or not parsed_url.netloc:
            messagebox.showerror("Invalid URL", "Enter a complete HTTP or HTTPS video URL.")
            return
        if not output.is_dir():
            messagebox.showerror("Output required", "Select an existing output folder first.")
            return
        ytdlp = shutil.which("yt-dlp")
        if ytdlp is None:
            messagebox.showerror("yt-dlp not found", "Install yt-dlp separately and make it available on PATH to use URL conversion.")
            return
        self._start_worker(lambda options: self._download_and_convert(ytdlp, url, output, options))

    def _start_worker(self, operation: Callable[[ConversionOptions], None]) -> None:
        """Start one background conversion after validating its settings."""
        if self.worker is not None and self.worker.is_alive():
            return
        try:
            options = self._validated_options()
            ffmpeg = shutil.which("ffmpeg")
            if ffmpeg is None:
                raise ValueError("FFmpeg was not found on PATH. Install it separately before converting.")
        except (ValueError, KeyError) as error:
            messagebox.showerror("Invalid settings", str(error))
            return
        if not self.output_dir.get() or not Path(self.output_dir.get()).is_dir():
            messagebox.showerror("Output required", "Select an existing output folder first.")
            return
        self.ffmpeg_path = ffmpeg
        self.stop_event.clear()
        self.convert_button.configure(state=tk.DISABLED)
        self.stop_button.configure(state=tk.NORMAL)
        self.status.set("Conversion is running.")

        def run() -> None:
            """Run the selected operation without blocking Tk's event loop."""
            try:
                operation(options)
                if not self.stop_event.is_set():
                    self.events.put(("status", "Conversion finished."))
            except InterruptedError:
                self.events.put(("status", "Conversion cancelled."))
            except Exception as error:  # Report worker errors in the UI.
                self.events.put(("log", f"Error: {error}"))
                self.events.put(("status", "Conversion stopped after an error."))
            finally:
                self.events.put(("done", ""))

        self.worker = threading.Thread(target=run, daemon=True)
        self.worker.start()

    def _convert_folder(self, source: Path, output: Path, options: ConversionOptions) -> None:
        """Convert supported videos directly inside the selected source folder."""
        videos = sorted(path for path in source.iterdir() if path.is_file() and path.suffix.lower() in VIDEO_SUFFIXES)
        if not videos:
            self.events.put(("log", f"No supported video files found in {source}."))
            return
        for channel_number, video in enumerate(videos, start=1):
            if self.stop_event.is_set():
                break
            self._convert_file(video, output, channel_number, options)

    def _download_and_convert(self, ytdlp: str, url: str, output: Path, options: ConversionOptions) -> None:
        """Download a URL to a temporary desktop folder and convert the result."""
        with tempfile.TemporaryDirectory(prefix="minitv-download-") as download_dir:
            template = str(Path(download_dir) / "%(title).120s.%(ext)s")
            command = [
                ytdlp, "--no-playlist", "--ffmpeg-location", self.ffmpeg_path,
                "--merge-output-format", "mp4", "-o", template, url,
            ]
            self._run_command(command, "Downloading video")
            videos = sorted(path for path in Path(download_dir).iterdir() if path.is_file() and path.suffix.lower() in VIDEO_SUFFIXES)
            if not videos:
                raise RuntimeError("yt-dlp completed without producing a supported video file.")
            for channel_number, video in enumerate(videos, start=1):
                if self.stop_event.is_set():
                    break
                self._convert_file(video, output, channel_number, options)

    def _convert_file(self, video: Path, output: Path, channel_number: int, options: ConversionOptions) -> None:
        """Encode one numbered channel pair and preserve existing channel files."""
        channel = output / str(channel_number)
        video_target = channel / f"{video.stem}.mjpeg"
        audio_target = channel / f"{video.stem}.mp3"
        if video_target.exists() or audio_target.exists():
            self.events.put(("log", f"Skipping {video.name}: output already exists in {channel}."))
            return
        channel.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix=".minitv-stage-", dir=output) as stage:
            stage_dir = Path(stage)
            staged_video = stage_dir / video_target.name
            staged_audio = stage_dir / audio_target.name
            scale = f"scale={options.width}:{options.height}:force_original_aspect_ratio=increase"
            crop = f"crop={options.width}:{options.height},setsar=1,fps={options.fps}"
            video_filter = f"{scale},{crop}"
            self.events.put(("log", f"Encoding {video.name} as {options.width}x{options.height} at {options.fps} fps."))
            self._run_command([
                self.ffmpeg_path, "-nostdin", "-hide_banner", "-loglevel", "error",
                "-i", str(video), "-an", "-vf", video_filter, "-q:v", str(options.quality),
                "-f", "mjpeg", "-y", str(staged_video),
            ], f"Encoding video {video.name}")
            self._run_command([
                self.ffmpeg_path, "-nostdin", "-hide_banner", "-loglevel", "error",
                "-i", str(video), "-vn", "-ar", str(options.audio_rate), "-ac", "1",
                "-c:a", "libmp3lame", "-b:a", "32k", "-af", f"volume={options.volume_db}dB",
                "-y", str(staged_audio),
            ], f"Encoding audio {video.name}")
            if self.stop_event.is_set():
                return
            if video_target.exists() or audio_target.exists():
                self.events.put(("log", f"Skipping publish for {video.name}: channel output appeared during conversion."))
                return
            os.replace(staged_video, video_target)
            os.replace(staged_audio, audio_target)
        self.events.put(("log", f"Created {video_target} and {audio_target}."))

    def _run_command(self, command: list[str], description: str) -> None:
        """Run an external media tool without a shell and capture its errors."""
        if self.stop_event.is_set():
            raise InterruptedError("Stop requested.")
        self.events.put(("log", f"{description}…"))
        process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        with self.process_lock:
            self.process = process
        _stdout, stderr = process.communicate()
        with self.process_lock:
            self.process = None
        if self.stop_event.is_set():
            raise InterruptedError("Stop requested.")
        if process.returncode:
            detail = (stderr or "").strip()
            raise RuntimeError(f"{description} failed (exit {process.returncode}). {detail[-2000:]}")

    def _request_stop(self) -> None:
        """Request cancellation and terminate only this tool's active child."""
        self.stop_event.set()
        with self.process_lock:
            process = self.process
        if process is not None and process.poll() is None:
            process.terminate()
        self.status.set("Stopping after the active media command exits…")
        self.stop_button.configure(state=tk.DISABLED)

    def _poll_events(self) -> None:
        """Apply background status and log events on Tk's main thread."""
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "log":
                self.log.configure(state="normal")
                self.log.insert(tk.END, value + "\n")
                self.log.see(tk.END)
                self.log.configure(state="disabled")
            elif kind == "status":
                self.status.set(value)
            elif kind == "done":
                self.convert_button.configure(state=tk.NORMAL)
                self.stop_button.configure(state=tk.DISABLED)
        self.root.after(100, self._poll_events)


def main() -> None:
    """Start the MiniTV converter desktop application."""
    root = tk.Tk()
    MiniTVConverter(root)
    root.mainloop()


if __name__ == "__main__":
    main()
