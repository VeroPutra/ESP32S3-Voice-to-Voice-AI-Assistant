from flask import Flask, request, jsonify
import speech_recognition as sr
import numpy as np
import io
import wave
import tempfile
import os

app = Flask(__name__)

# Initialize speech recognizer
recognizer = sr.Recognizer()

def convert_raw_to_wav(raw_data, sample_rate=16000, channels=1, sample_width=2):
    """Convert raw audio data to WAV format"""
    try:
        # Create a temporary WAV file
        with tempfile.NamedTemporaryFile(delete=False, suffix='.wav') as temp_file:
            # Create WAV file
            with wave.open(temp_file.name, 'wb') as wav_file:
                wav_file.setnchannels(channels)
                wav_file.setsampwidth(sample_width)
                wav_file.setframerate(sample_rate)
                wav_file.writeframes(raw_data)
            
            return temp_file.name
    except Exception as e:
        print(f"Error converting to WAV: {e}")
        return None

@app.route('/upload', methods=['POST'])
def upload_audio():
    try:
        # Get audio data from request
        audio_data = request.get_data()
        
        if len(audio_data) == 0:
            return jsonify({'error': 'No audio data received'}), 400
        
        # Get audio parameters from headers
        sample_rate = int(request.headers.get('X-Sample-Rate', 11000))
        channels = int(request.headers.get('X-Channels', 1))
        bits_per_sample = int(request.headers.get('X-Bits-Per-Sample', 1))
        sample_width = bits_per_sample // 8
        
        print(f"Received audio: {len(audio_data)} bytes")
        print(f"Sample rate: {sample_rate}, Channels: {channels}, Bits: {bits_per_sample}")
        
        # Convert raw audio to WAV
        wav_filename = convert_raw_to_wav(audio_data, sample_rate, channels, sample_width)
        
        if wav_filename is None:
            return jsonify({'error': 'Failed to convert audio'}), 500
        
        try:
            # Perform speech recognition
            with sr.AudioFile(wav_filename) as source:
                # Adjust for ambient noise
                recognizer.adjust_for_ambient_noise(source, duration=0.5)
                # Record the audio
                audio = recognizer.record(source)
            
            # Try to recognize speech using Google Speech Recognition
            try:
                text = recognizer.recognize_google(audio, language='id-ID')  # Indonesian
                print(f"Recognized text: {text}")
                
                response = {
                    'success': True,
                    'text': text,
                    'confidence': 'high'
                }
                
            except sr.UnknownValueError:
                print("Could not understand audio")
                response = {
                    'success': False,
                    'text': '',
                    'error': 'Could not understand audio'
                }
                
            except sr.RequestError as e:
                print(f"Error with speech recognition service: {e}")
                # Fallback to offline recognition
                try:
                    text = recognizer.recognize_sphinx(audio)
                    print(f"Offline recognition: {text}")
                    response = {
                        'success': True,
                        'text': text,
                        'confidence': 'medium',
                        'method': 'offline'
                    }
                except:
                    response = {
                        'success': False,
                        'text': '',
                        'error': f'Speech recognition service error: {e}'
                    }
        
        finally:
            # Clean up temporary file
            if os.path.exists(wav_filename):
                os.unlink(wav_filename)
        
        return jsonify(response)
    
    except Exception as e:
        print(f"Server error: {e}")
        return jsonify({'error': f'Server error: {str(e)}'}), 500

@app.route('/test', methods=['GET'])
def test():
    return jsonify({
        'status': 'Server is running',
        'endpoints': ['/upload (POST)', '/test (GET)']
    })

if __name__ == '__main__':
    print("Starting Speech Recognition Server...")
    print("Make sure you have installed the required packages:")
    print("pip install flask speechrecognition pyaudio numpy")
    print("For offline recognition, also install:")
    print("pip install pocketsphinx")
    print("\nServer will run on http://0.0.0.0:2401")
    print("Update ESP32 code with your PC's IP address")
    
    app.run(host='0.0.0.0', port=2401, debug=True)