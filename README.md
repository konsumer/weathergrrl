This is a simple weather-forecast device.

```sh
pip install platformio

pio run --target upload
```



## notes

```
# location
https://vpncheck-david-konsumers-projects-edc64e5e.vercel.app/geo

# weather
https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,surface_pressure,pressure_msl,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m&timezone=auto&forecast_days=3
```