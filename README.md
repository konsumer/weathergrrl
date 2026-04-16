This is a simple weather-forecast device.

<img width="523" height="393" alt="Screenshot 2026-04-16 at 6 06 41 AM" src="https://github.com/user-attachments/assets/79cd8287-1976-43c2-910f-b6b978c471bf" />

```sh
pip install platformio

pio run --target upload
```

Build this with a [CYD](https://www.aliexpress.us/w/wholesale-cyd.html), which you can usually get for about $20.

The default config is to use wifi to check weather and display it, but this idea could be extended to read sensors or display some other internet thing. If you hold your finger on screen while it boots, it will reset the wifi info.

Shows current temp, "feels like" temp, humidity, pressure, wind (speed & direction, gusts) and sun rise/set times, as well as forecast for the week, and what clothes are approprate.