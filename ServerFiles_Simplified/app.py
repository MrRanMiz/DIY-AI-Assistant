"""
Simplified ESP32 AI Voice Assistant Server - Text Response Only
No audio output, just transcription and text response
~80 lines - Path A (API-based)
"""

from flask import Flask, request, jsonify
import requests
import os
import logging
import tempfile
from huggingface_hub import InferenceClient
from groq import Groq

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size

# Get API tokens from environment
HF_TOKEN = os.environ.get('HF_TOKEN', '')
GROQ_API_KEY = os.environ.get('GROQ_API_KEY', '')

# Initialize API clients
hf_client = InferenceClient(token=HF_TOKEN) if HF_TOKEN else InferenceClient()
groq_client = Groq(api_key=GROQ_API_KEY) if GROQ_API_KEY else None

@app.route("/")
def home():
    """Home page"""
    return """
    <html>
        <head><title>ESP32 Voice Assistant</title></head>
        <body style="font-family: Arial; padding: 20px;">
            <h1>🎤 ESP32 AI Voice Assistant Server</h1>
            <p>Status: <span style="color: green;">Running</span></p>
            <h3>Endpoints:</h3>
            <ul>
                <li><code>GET /status</code> - Check server status</li>
                <li><code>POST /process_audio</code> - Process audio and get text response</li>
            </ul>
            <p><small>Simplified version - Text response only (no audio output)</small></p>
        </body>
    </html>
    """

@app.route("/status")
def status():
    """Status endpoint - check if server is ready"""
    logger.info("Status check requested")
    return jsonify({
        "ready": True,
        "status": "ok",
        "mode": "text-only",
        "models": {
            "stt": "whisper-base (API)",
            "llm": "flan-t5-base (API)",
            "tts": "none (text-only mode)"
        }
    })

@app.route("/process_audio", methods=["POST"])
def process_audio():
    """
    Main endpoint - receives audio, returns text response
    Expected: WAV audio file in request body
    Returns: JSON with transcript and AI response
    """
    try:
        # Get audio data from request
        audio_data = request.data
        
        if not audio_data:
            logger.error("No audio data received")
            return jsonify({"error": "No audio data"}), 400
        
        logger.info(f"Received audio data: {len(audio_data)} bytes")
        
        # Step 1: Transcribe audio to text
        logger.info("Transcribing audio...")
        transcript = transcribe_audio(audio_data)
        logger.info(f"Transcript: {transcript}")
        
        # Step 2: Generate AI response
        logger.info("Generating response...")
        response_text = generate_response(transcript)
        logger.info(f"Response: {response_text}")
        
        # Return both transcript and response
        return jsonify({
            "success": True,
            "transcript": transcript,
            "response": response_text
        })
        
    except Exception as e:
        logger.error(f"Error processing audio: {str(e)}")
        return jsonify({
            "success": False,
            "error": str(e)
        }), 500

def transcribe_audio(audio_bytes):
    """
    Transcribe audio using HuggingFace Whisper via InferenceClient
    """
    try:
        logger.info("Using HuggingFace InferenceClient for transcription...")
        
        # Save audio bytes to a temporary WAV file
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as temp_audio:
            temp_audio.write(audio_bytes)
            temp_path = temp_audio.name
        
        try:
            # Use the temporary file path for transcription
            result = hf_client.automatic_speech_recognition(temp_path)
        finally:
            # Clean up the temporary file
            if os.path.exists(temp_path):
                os.unlink(temp_path)
        
        logger.info(f"Whisper result: {result}")
        
        if result and isinstance(result, dict) and "text" in result:
            text = result["text"].strip()
        elif result and isinstance(result, str):
            text = result.strip()
        else:
            text = str(result).strip() if result else ""
        
        if not text or len(text) < 2:
            return "Sorry, I couldn't understand that."
        
        return text
            
    except Exception as e:
        logger.error(f"Transcription error: {type(e).__name__}: {str(e)}")
        import traceback
        logger.error(f"Traceback: {traceback.format_exc()}")
        return "Sorry, transcription failed."

def generate_response(text):
    """
    Generate AI response using Groq API (llama-3.1-8b-instant)
    """
    try:
        if not text or text in ["Sorry, I couldn't understand that.", "Sorry, transcription failed."]:
            return "I'm listening. Please speak clearly."
        
        logger.info(f"Generating response for: {text}")
        
        # Check if Groq client is available
        if not groq_client:
            logger.warning("GROQ_API_KEY not set, using fallback responses")
            raise ValueError("Groq API not configured")
        
        # Use Groq API for conversational AI
        chat_completion = groq_client.chat.completions.create(
            messages=[
                {
                    "role": "system",
                    "content": (
                        "You are a helpful AI voice assistant for an ESP32 device with limited display. "
                        "Keep responses SHORT (1-2 sentences max, under 100 characters when possible). "
                        "Be conversational and friendly. Do NOT generate code, long explanations, or lists. "
                        "Just provide brief, natural conversational responses."
                    )
                },
                {
                    "role": "user",
                    "content": text
                }
            ],
            model="llama-3.1-8b-instant",
            max_tokens=100,  # Limit response length
            temperature=0.7  # Natural conversation
        )
        
        response = chat_completion.choices[0].message.content.strip()
        logger.info(f"Groq response: {response}")
        return response
            
    except Exception as e:
        logger.error(f"Response generation error: {type(e).__name__}: {str(e)}")
        # Fallback to simple pattern matching if Groq fails
        text_lower = text.lower()
        if "hello" in text_lower or "hi" in text_lower:
            return "Hello! How can I help you?"
        elif "thank" in text_lower:
            return "You're welcome!"
        else:
            return "I heard you!"

if __name__ == "__main__":
    logger.info("Starting ESP32 Voice Assistant Server (Text-Only Mode)")
    logger.info(f"HF_TOKEN configured: {bool(HF_TOKEN)}")
    
    # Get port from environment or use default
    port = int(os.environ.get("PORT", 7860))
    
    app.run(host="0.0.0.0", port=port, debug=False)
