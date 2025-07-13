from flask import Flask, request, send_file, jsonify, Response
import os
import time
import threading
import shutil
from pathlib import Path
import io

# gTTS imports
from gtts import gTTS
import tempfile

# Optional audio playback (comment out if not needed)
try:
    from pydub import AudioSegment
    import simpleaudio as sa
    AUDIO_PLAYBACK_AVAILABLE = True
    
    # Auto-setup ffmpeg for pydub
    try:
        import imageio_ffmpeg
        ffmpeg_path = imageio_ffmpeg.get_ffmpeg_exe()
        AudioSegment.converter = ffmpeg_path
        print(f"[INFO] ffmpeg path for pydub is set: {ffmpeg_path}")
    except ImportError:
        print("[WARNING] imageio-ffmpeg not installed, ensure ffmpeg is available in PATH")
    except Exception as e:
        print("[WARNING] imageio-ffmpeg auto-setup failed:", e)
        
except ImportError:
    print("[INFO] Audio playback libraries not available (pydub/simpleaudio)")
    AUDIO_PLAYBACK_AVAILABLE = False

# ==== KONFIGURASI ====
SERVER_IP = "192.168.88.240"    # Your server IP
TTS_CHAR_LIMIT = 1000
AUDIOFILE = "audio.mp3"
AUDIO_DIR = "audio_files"
GTTS_LANGUAGE = "id"  # Indonesian language
GTTS_SLOW = False     # Speech speed (False = normal, True = slow)
# ======================

app = Flask(__name__)

# Create audio directory if it doesn't exist
Path(AUDIO_DIR).mkdir(exist_ok=True)

def generate_tts_with_gtts(text, output_path):
    """Generate TTS using gTTS (Google Text-to-Speech)"""
    try:
        print(f"[INFO] Generating TTS with gTTS for text: '{text[:50]}{'...' if len(text) > 50 else ''}'")
        
        # Create gTTS object
        tts = gTTS(
            text=text,
            lang=GTTS_LANGUAGE,
            slow=GTTS_SLOW,
            lang_check=True,
            pre_processor_funcs=[
                # Add text preprocessing if needed
                lambda x: x.replace('\n', ' ').replace('\r', ' ')
            ]
        )
        
        # Save to temporary file first
        with tempfile.NamedTemporaryFile(delete=False, suffix='.mp3') as tmp_file:
            temp_path = tmp_file.name
            
        # Write to temp file
        tts.save(temp_path)
        
        # Move to final location
        if os.path.exists(temp_path):
            if os.path.exists(output_path):
                shutil.move(output_path)
            shutil.move(temp_path, output_path)
            
            file_size = os.path.getsize(output_path)
            print(f"[SUCCESS] gTTS audio generated: {file_size} bytes")
            return True
        else:
            print("[ERROR] gTTS failed to generate audio file")
            return False
            
    except Exception as e:
        print(f"[ERROR] gTTS generation failed: {e}")
        # Clean up temp file if it exists
        try:
            if 'temp_path' in locals() and os.path.exists(temp_path):
                shutil.move(temp_path)
        except:
            pass
        return False

def optimize_audio_for_esp32(input_path, output_path):
    """Optimize audio specifically for ESP32 playback"""
    try:
        print("[INFO] Optimizing audio for ESP32...")
        
        # Load audio
        audio = AudioSegment.from_mp3(input_path)
        
        # Convert to optimal format for ESP32
        audio = audio.set_frame_rate(16000)  # Standard sample rate
        audio = audio.set_channels(1)        # Mono
        audio = audio.set_sample_width(2)    # 16-bit
        
        # Normalize audio to prevent clipping
        audio = audio.normalize()
        
        # Apply slight compression to reduce dynamic range
        # This helps with consistent playback on ESP32
        audio = audio.compress_dynamic_range(threshold=-20.0, ratio=2.0, attack=1.0, release=10.0)
        
        # Export with ESP32-optimized settings
        audio.export(
            output_path,
            format="mp3",
            bitrate="64k",           # Low bitrate for ESP32
            parameters=[
                "-ac", "1",          # Mono
                "-ar", "16000",      # Sample rate
                "-acodec", "mp3",    # MP3 codec
                "-q:a", "4",         # High quality encoding
                "-joint_stereo", "0", # Disable joint stereo
                "-reservoir", "0"     # Disable bit reservoir for consistent bitrate
            ]
        )
        
        # Verify the output
        optimized = AudioSegment.from_mp3(output_path)
        print(f"[INFO] Optimized audio: {optimized.frame_rate}Hz, {optimized.channels}ch, {len(optimized)}ms")
        
        return True
        
    except Exception as e:
        print(f"[ERROR] Audio optimization failed: {e}")
        return False

def play_audio_locally(filepath):
    """Play audio file locally on server (non-blocking)"""
    if not AUDIO_PLAYBACK_AVAILABLE:
        print("[INFO] Local audio playback not available")
        return
        
    try:
        print("[INFO] Playing audio locally...")
        song = AudioSegment.from_mp3(filepath)
        playback = sa.play_buffer(
            song.raw_data,
            num_channels=song.channels,
            bytes_per_sample=song.sample_width,
            sample_rate=song.frame_rate
        )
        playback.wait_done()
        print("[INFO] Local audio playback finished")
    except Exception as e:
        print(f"[ERROR] Local audio playback failed: {e}")

@app.route("/", methods=["GET"])
def index():
    return """
    <h1>Flask gTTS Backend - ESP32 Optimized</h1>
    <p>Status: <strong>Active</strong></p>
    <p>TTS Engine: <strong>Google Text-to-Speech (gTTS)</strong></p>
    <p>Language: <strong>{}</strong></p>
    <p>POST to <code>/tts</code> with JSON: <code>{{"text": "your text"}}</code></p>
    <p>GET <code>/audio.mp3</code> to download latest audio file</p>
    <p>Character limit: {}</p>
    <p>Audio format: 16000Hz, Mono, 64kbps MP3</p>
    """.format(GTTS_LANGUAGE, TTS_CHAR_LIMIT)

@app.route("/tts", methods=["POST"])
def tts():
    try:
        print("\n=== New TTS Request ===")
        data = request.json
        if not data or not data.get("text"):
            return jsonify({"error": "text field required"}), 400

        text = data["text"].strip()
        original_length = len(text)
        
        if len(text) > TTS_CHAR_LIMIT:
            text = text[:TTS_CHAR_LIMIT]
            print(f"[INFO] Text truncated from {original_length} to {len(text)} characters")

        if not text:
            return jsonify({"error": "empty text after processing"}), 400

        print(f"[INFO] Processing text: '{text[:50]}{'...' if len(text) > 50 else ''}'")

        # Generate TTS using gTTS
        raw_audio_path = os.path.join(AUDIO_DIR, "raw_" + AUDIOFILE)
        optimized_audio_path = os.path.join(AUDIO_DIR, AUDIOFILE)
        
        # Generate with gTTS
        start_time = time.time()
        tts_success = generate_tts_with_gtts(text, raw_audio_path)
        generation_time = time.time() - start_time
        
        if not tts_success:
            return jsonify({"error": "TTS generation failed"}), 500

        raw_file_size = os.path.getsize(raw_audio_path)
        print(f"[INFO] gTTS generation took: {generation_time:.2f}s")
        print(f"[INFO] Raw audio saved: {raw_file_size} bytes")
        
        if raw_file_size < 1000:
            return jsonify({"error": "Generated audio file too small"}), 500

        # Optimize audio for ESP32 if pydub is available
        optimization_success = False
        if AUDIO_PLAYBACK_AVAILABLE:
            optimization_success = optimize_audio_for_esp32(raw_audio_path, optimized_audio_path)
        
        # If optimization failed, use raw file
        if not optimization_success:
            print("[WARNING] Audio optimization failed, using raw file")
            os.replace(raw_audio_path, optimized_audio_path)
        else:
            # Remove raw file if optimization succeeded
            os.remove(raw_audio_path)
        
        final_file_size = os.path.getsize(optimized_audio_path)
        total_time = time.time() - start_time
        print(f"[SUCCESS] Total processing time: {total_time:.2f}s")
        print(f"[SUCCESS] Final audio ready: {final_file_size} bytes")
        
        # Wait for file to be completely written and flushed
        time.sleep(0.5)
        
        # Play locally in background thread (non-blocking)
        if AUDIO_PLAYBACK_AVAILABLE:
            threading.Thread(target=play_audio_locally, args=(optimized_audio_path,), daemon=True).start()

        # Return URL for ESP32
        audio_url = f"http://{SERVER_IP}:2400/audio.mp3"
        print(f"[INFO] Returning URL: {audio_url}")
        
        return jsonify({
            "url": audio_url,
            "text_length": len(text),
            "file_size": final_file_size,
            "optimized": optimization_success,
            "format": "16000Hz Mono MP3",
            "engine": "gTTS",
            "language": GTTS_LANGUAGE,
            "generation_time": round(generation_time, 2),
            "total_time": round(total_time, 2),
            "status": "success"
        })
        
    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({"error": "Internal Server Error", "exception": str(e)}), 500

@app.route("/audio.mp3")
def get_audio():
    audio_path = os.path.join(AUDIO_DIR, AUDIOFILE)
    if os.path.exists(audio_path):
        file_size = os.path.getsize(audio_path)
        print(f"[INFO] Serving audio file: {file_size} bytes")
        
        # Enhanced headers for ESP32 compatibility
        response = send_file(
            audio_path, 
            mimetype="audio/mpeg",
            as_attachment=False,
            download_name="audio.mp3"
        )
        
        # Optimized headers for ESP32 HTTP streaming
        response.headers['Content-Type'] = 'audio/mpeg'
        response.headers['Content-Length'] = str(file_size)
        response.headers['Accept-Ranges'] = 'bytes'
        response.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
        response.headers['Pragma'] = 'no-cache'
        response.headers['Expires'] = '0'
        response.headers['Connection'] = 'close'  # Prevent keep-alive issues
        
        return response
    else:
        print("[ERROR] Audio file not found")
        return jsonify({"error": "Audio file not found"}), 404

@app.route("/test", methods=["GET"])
def test_endpoint():
    """Test endpoint to check if server is working"""
    audio_path = os.path.join(AUDIO_DIR, AUDIOFILE)
    audio_exists = os.path.exists(audio_path)
    audio_size = os.path.getsize(audio_path) if audio_exists else 0
    
    return jsonify({
        "status": "server running",
        "tts_engine": "gTTS (Google Text-to-Speech)",
        "language": GTTS_LANGUAGE,
        "audio_available": audio_exists,
        "audio_size": audio_size,
        "tts_limit": TTS_CHAR_LIMIT,
        "local_playback": AUDIO_PLAYBACK_AVAILABLE,
        "audio_url": f"http://{SERVER_IP}:2400/audio.mp3",
        "optimization_available": AUDIO_PLAYBACK_AVAILABLE,
        "recommended_format": "16000Hz Mono MP3"
    })

@app.route("/test-audio")
def test_audio():
    """Direct test of audio file with proper headers and chunked streaming"""
    audio_path = os.path.join(AUDIO_DIR, AUDIOFILE)
    if not os.path.exists(audio_path):
        return "No audio file available", 404
    
    file_size = os.path.getsize(audio_path)
    
    def generate():
        with open(audio_path, 'rb') as f:
            # Use smaller chunks for better ESP32 compatibility
            chunk_size = 4096
            while True:
                data = f.read(chunk_size)
                if not data:
                    break
                yield data
    
    return Response(
        generate(),
        mimetype="audio/mpeg",
        headers={
            "Content-Type": "audio/mpeg",
            "Content-Length": str(file_size),
            "Accept-Ranges": "bytes",
            "Cache-Control": "no-cache",
            "Connection": "close"
        }
    )

# Test gTTS endpoint
@app.route("/test-tts", methods=["GET"])
def test_tts():
    """Quick test of gTTS functionality"""
    test_text = "Halo, ini adalah test audio dari gTTS"
    try:
        result = generate_tts_with_gtts(test_text, os.path.join(AUDIO_DIR, "test_" + AUDIOFILE))
        if result:
            return jsonify({
                "status": "gTTS test successful", 
                "text": test_text,
                "engine": "gTTS",
                "language": GTTS_LANGUAGE
            })
        else:
            return jsonify({"status": "gTTS test failed"}), 500
    except Exception as e:
        return jsonify({"status": "gTTS test error", "error": str(e)}), 500

if __name__ == '__main__':
    print("=== Flask gTTS Backend - ESP32 Optimized ===")
    print(f"TTS Engine: Google Text-to-Speech (gTTS)")
    print(f"Language: {GTTS_LANGUAGE}")
    print(f"Server IP: {SERVER_IP}")
    print(f"TTS Character Limit: {TTS_CHAR_LIMIT}")
    print(f"Audio Directory: {AUDIO_DIR}")
    print(f"Local Playback: {AUDIO_PLAYBACK_AVAILABLE}")
    print(f"Audio Optimization: {AUDIO_PLAYBACK_AVAILABLE}")
    print("===============================================")
    
    app.run(host="0.0.0.0", port=2400, debug=False, threaded=True)