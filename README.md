# BigClown bcp-wireless-circus

Firmware for projects

* [Smart LED Strip](https://doc.bigclown.cz/smart-led-strip.html)
* [Jednoduché programování domácí automatizace](https://doc.bigclown.cz/easy-programming.html)
* [Workroom project](https://www.bigclown.com/project/lets-build-your-own-home-automation/)

## Tags
* [Humidity Tag](https://obchod.bigclown.cz/products/humidity-tag)
* [Lux Meter Tag](https://obchod.bigclown.cz/products/lux-meter-tag)
* [Barometer Tag](https://obchod.bigclown.cz/products/barometr-tag)
* [Temperature Tag](https://obchod.bigclown.cz/products/temperature-tag)

## Remote
* 1x [BigClown Core Module](https://obchod.bigclown.cz/products/core-module)
* 1x [Tag Module](https://obchod.bigclown.cz/products/tag-module)
* 1x [BigClown Battery Module](https://obchod.bigclown.cz/products/battery-module)
* Tags

![](images/unit-remote.png)

## Base
* 1x [BigClown Core Module](https://obchod.bigclown.cz/products/core-module)
* 1x [BigClown Power Module](https://obchod.bigclown.cz/products/power-module)
* 1x [BigClown Base Module](https://obchod.bigclown.cz/products/base-module)
* Tags

![](images/unit-base.png)

## Hub (Gateway)
* 1x [BigClown Raspberry Pi](https://obchod.bigclown.cz/products/raspberry-pi-3-set)


### MQTT

##### LED strip

  * On
    ```
    mosquitto_pub -t "nodes/base/light/-/set" -m '{"state": true}'
    ```
  * Off
    ```
    mosquitto_pub -t "nodes/base/light/-/set" -m '{"state": false}'
    ```
  * Get state
    ```
    mosquitto_pub -t "nodes/base/light/-/get" -m '{}'
    ```
  * For 144 x RGBW LED strip, set all the lights on the red, data are encoded in base64
    ```
    mosquitto_pub -t 'nodes/base/led-strip/-/set' -m  '{"pixels": "/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA/wAAAP8AAAD/AAAA"}'
    ```
  * Config
    * [LED Strip RGBW 1m 144 LEDs](https://shop.bigclown.com/products/led-stripe-rgbw-1m-144leds-glue)
      ```
      mosquitto_pub -t 'nodes/base/led-strip/-/config/set' -m '{"mode": "rgbw", "count": 144}'
      ```
    * [LED Strip RGB 5m 150 LEDs](https://shop.bigclown.com/products/led-stripe-5m)
      ```
      mosquitto_pub -t 'nodes/base/led-strip/-/config/set' -m '{"mode": "rgb", "count": 150}'
      ```

#### Relátko na power modulu
  * On
    ```
    mosquitto_pub -t "nodes/base/relay/-/set" -m '{"state": true}'
    ```
    > **Hint** First aid:
    If the relay not clicked, so make sure you join 5V DC adapter to Power Module

  * Off
    ```
    mosquitto_pub -t "nodes/base/relay/-/set" -m '{"state": false}'
    ```
  * Get state
    ```
    mosquitto_pub -t "nodes/base/relay/-/get" -m '{}'
    ```
