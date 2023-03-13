# Matrix Display - Clock and Text scrolling
This little piece of Hard- and software was inspired by [Qromes marquee scroller](https://github.com/Qrome/marquee-scroller). I used the mentioned components, printed the case but uploaded my little piece of software onto the ESP.

## Functions
First of all, it's a clock. The default Timezone will be set to Europe/Berlin and the Time will be fetched from a few timeservers after connecting to your WiFi Network. The choosen Hostname is displayed once after connecting to your WiFi (something with ESP-XXXXXXX)

Thanks to a cheap build in HTTP based API, you have two more options:
1. Scoll a Text
2. Display a short animation

### How to scroll a Text
With a easy to use commandline tool `curl` you can scroll a text:

```
curl -X POST http://ESP-XXXXXX/ \
   -H "Content-Type: application/x-www-form-urlencoded" \
   -d "message=Hallo Welt"
```

The output of this command will be `Requests handled, thank you!` if everything worked. You should also see the text scrolling once.

### How to show a animation
There a currently 9 different animations implemented (see `effects.cpp`). You can choose between them via curl as well"

```
curl -X POST http://ESP-XXXXXX/ \
   -H "Content-Type: application/x-www-form-urlencoded" \
   -d "animation=1"
```

### Using iOS or MacOS shortcuts or Home automations
#### Shortcuts
I've build some easy to use iOS/MacOS shortcuts to scroll a message or display an animation on one or more of such displays. The shortscuts call eachother for success.

- Scroll a Text: https://www.icloud.com/shortcuts/00ed297f36e14bc3b652645d98c085d6
- Display an Animation: https://www.icloud.com/shortcuts/e692ef97914e49ce975b7a721d22b87e
- Dependencie for Animations: https://www.icloud.com/shortcuts/43bb0c42650a49e9aec371fbedc436c8
- Send the request via HTTP: https://www.icloud.com/shortcuts/4048d1cc4fac45dfbf8cb702ef862c6d

Why so complicated? I've multiple of such displays in my home and with this structure I can select between them.

#### Home Automation
Create a new automation in your Home App, select a trigger and on the next page select "Create shortcut" on the very bottom of the list of devices. Now you can create a (very limited) shortcut. This example pulls the current temperature from a Homepod and scrolls it as a message:

![Example](/docs/automation.png)

## Configure WiFi
TBD
