# ESP32 AI Voice Assistant - Simplified Server (Text-Only)

This is a simplified version of the server that returns **text responses only** (no audio output).

Perfect for when you don't have a speaker/amplifier connected to your ESP32.

## Files in This Folder

- **app.py** - Main server code (~170 lines, simple!)
- **requirements.txt** - Python packages (only 2!)
- **Dockerfile** - Docker configuration for HuggingFace deployment
- **test_local.ps1** - PowerShell script to test locally
- **README.md** - This file

---

## Quick Start - Local Testing (Recommended First!)

### Step 1: Install Python Dependencies

```powershell
cd ServerFiles_Simplified
pip install -r requirements.txt
```

### Step 2: Run Server Locally

```powershell
python app.py
```

You should see:
```
INFO:__main__:Starting ESP32 Voice Assistant Server (Text-Only Mode)
INFO:werkzeug: * Running on http://0.0.0.0:7860
```

### Step 3: Test It

**Open a NEW PowerShell window** and run:

```powershell
.\test_local.ps1
```

This will:
- ✓ Check if server is running
- ✓ Open browser to view status
- ✓ Send test audio (if you have test_audio.wav)

---

## Deploy to HuggingFace Space

Once local testing works, deploy to HuggingFace:

### Step 1: Create HuggingFace Space

1. Go to https://huggingface.co/spaces
2. Click **"Create new Space"**
3. Settings:
   - **Name:** `esp32-voice-assistant` (or your choice)
   - **SDK:** Docker
   - **Hardware:** CPU basic (FREE tier)
   - **Visibility:** Public or Private

### Step 2: Upload Files

Click **Files** tab → **Add file** → Upload these 3 files:
- `app.py`
- `requirements.txt`
- `Dockerfile`

### Step 3: Set HuggingFace Token (Important!)

1. Go to your HuggingFace profile → **Settings** → **Access Tokens**
2. Create a new token (type: **Read**)
3. Copy the token
4. In your Space: **Settings** → **Repository secrets**
5. Add secret:
   - **Name:** `HF_TOKEN`
   - **Value:** Your token

### Step 4: Wait for Build

Watch the **Logs** tab. You'll see:
```
Building...
Installing dependencies...
Running...
Server ready ✓
```

Build time: ~2-5 minutes

### Step 5: Test Your Deployed Server

Your Space URL will be: `https://your-username-esp32-voice-assistant.hf.space`

**Test in browser:**
```
https://your-username-esp32-voice-assistant.hf.space/status
```

Should show:
```json
{
  "ready": true,
  "status": "ok",
  "mode": "text-only"
}
```

**Test from PowerShell:**
```powershell
Invoke-WebRequest -Uri "https://your-space-url.hf.space/status"
```

---

## API Endpoints

### GET /status
Check if server is ready

**Response:**
```json
{
  "ready": true,
  "status": "ok",
  "mode": "text-only",
  "models": {
    "stt": "whisper-base (API)",
    "llm": "flan-t5-base (API)",
    "tts": "none"
  }
}
```

### POST /process_audio
Send audio, get text response

**Request:**
- Method: POST
- Content-Type: audio/wav
- Body: WAV file (binary)

**Response:**
```json
{
  "success": true,
  "transcript": "what is the weather",
  "response": "I'm a voice assistant. I don't have access to weather data."
}
```

---

## Testing Without Arduino

### Create a test WAV file:

**Option 1: Record on Windows**
1. Open **Voice Recorder** app
2. Record a short message
3. Save as WAV
4. Copy to `ServerFiles_Simplified` folder
5. Rename to `test_audio.wav`

**Option 2: Use online converter**
1. Record on phone
2. Upload to online-convert.com
3. Convert to WAV
4. Download and rename to `test_audio.wav`

### Test with PowerShell:

```powershell
# Local server
Invoke-WebRequest -Uri "http://localhost:7860/process_audio" `
  -Method POST `
  -InFile "test_audio.wav" `
  -ContentType "audio/wav"

# Deployed server
Invoke-WebRequest -Uri "https://your-space.hf.space/process_audio" `
  -Method POST `
  -InFile "test_audio.wav" `
  -ContentType "audio/wav"
```

---

## Troubleshooting

### "Server not running" error
- Make sure you ran `python app.py` first
- Check if port 7860 is already in use

### "API error" or slow responses
- HuggingFace API has rate limits (free tier)
- First request might take 20-30 seconds (model loading)
- Set `HF_TOKEN` environment variable for better limits

### "ModuleNotFoundError"
```powershell
pip install -r requirements.txt
```

### Check server logs
In HuggingFace Space → **Logs** tab shows all errors

---

## Differences from Original Server

| Feature | Original | Simplified |
|---------|----------|------------|
| Lines of code | 617 | 170 |
| Dependencies | 12 packages | 2 packages |
| Build time | 15 min | 2-5 min |
| Memory usage | 2-4GB | 500MB |
| Audio output | ✓ MP3 | ✗ Text only |
| Local models | ✓ Downloads | ✗ Uses API |
| File management | ✓ Complex | ✗ Not needed |
| Response time | Fast (local) | Medium (API) |

---

## Next Steps

1. ✓ Test locally
2. ✓ Deploy to HuggingFace
3. → Create Arduino sketch (text-only version)
4. → Connect ESP32 to server
5. → Display responses on OLED

---

## Notes

- Uses HuggingFace Inference API (no model downloads)
- Free tier has rate limits (~1000 requests/day)
- Response time: 5-15 seconds per request
- Works on free CPU tier (no GPU needed)
- Server may sleep after 48 hours inactivity (free tier)

---

## Support

If you get errors:
1. Check the **Logs** tab in HuggingFace
2. Make sure `HF_TOKEN` is set
3. Test locally first before deploying
4. Check that WAV file is valid (16kHz, mono recommended)
