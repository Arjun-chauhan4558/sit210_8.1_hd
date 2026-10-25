
import asyncio
import json
import queue
import numpy as np
import sounddevice as sd
from scipy.signal import resample_poly
from vosk import Model, KaldiRecognizer
from bleak import BleakScanner, BleakClient

CHAR_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

MIC_RATE = 48000
VOSK_RATE = 16000

audio_queue = queue.Queue()


def audio_callback(indata, frames, time, status):
    if status:
        print(status)

    audio_queue.put(bytes(indata))


def get_command(text):
    text = text.lower()

    if "light" in text and "off" in text:
        return "LIGHTS_OFF"
    elif "bathroom" in text:
        return "BATHROOM"
    elif "hallway" in text:
        return "HALLWAY"
    elif "fan" in text and "on" in text:
        return "FAN_ON"
    elif "fan" in text and "off" in text:
        return "FAN_OFF"

    return None


async def main():
    model = Model("model")
    recogniser = KaldiRecognizer(model, VOSK_RATE)

    print("Searching for Arduino Nano...")

    device = await BleakScanner.find_device_by_name("LightingNano")

    if device is None:
        print("Arduino not found.")
        return

    async with BleakClient(device) as client:
        print("Connected. Speak a command.")

        with sd.RawInputStream(
            samplerate=MIC_RATE,
            blocksize=4800,
            dtype="int16",
            channels=1,
            callback=audio_callback
        ):
            while True:
                audio_data = await asyncio.to_thread(
                    audio_queue.get
                )

                audio_48k = np.frombuffer(
                    audio_data, dtype=np.int16
                )

                audio_16k = resample_poly(
                    audio_48k, 1, 3
                ).astype(np.int16)

                if recogniser.AcceptWaveform(
                    audio_16k.tobytes()
                ):
                    result = json.loads(
                        recogniser.Result()
                    )

                    text = result.get("text", "")

                    if text:
                        print("Heard:", text)

                        command = get_command(text)

                        if command:
                            await client.write_gatt_char(
                                CHAR_UUID,
                                command.encode("utf-8")
                            )

                            print("Sent:", command)


asyncio.run(main())
