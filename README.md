This is a simple weather-forecast device.

<img width="521" height="392" alt="Screenshot 2026-04-16 at 5 55 27 AM" src="https://github.com/user-attachments/assets/ad57dc00-1bb0-4d45-9aa5-448bac6da786" />


```sh
pip install platformio

pio run --target upload
```

Build this with a [CYD](https://www.aliexpress.us/w/wholesale-cyd.html), which you can usually get for about $20.

The default config is to use wifi to check weather and display it, but this idea could be extended to read sensors or display some other internet thing.
