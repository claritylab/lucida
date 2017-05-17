Microsoft BotFramework interface to LUCIDA
==========================================

This interaface allows accessing LUCIDA services from BotFramework supported channels like Facebook, Skype, messaging etc. Currently only text infer queries are supported.

## Installation
* Change directory to $LUCIDAROOT/botframework-interface and run `make all`
* Create a new bot at https://dev.botframework.com/bots/new
    - Fill in Name, Bot handle and Description
    - Messaging endpoint is the address of the PC running the interface. This has to be https endpoint. For testing purposes one can use ngrok.
        * Install ngrok by typing `sudo apt-get install ngrok-client` in terminal
        * Type `ngrok http 3728` to start ngrok. The port 3728 should be replaced by appropriate port if interface is running on a different port.
        * You'll see a `https://*.ngrok.io` address when ngrok goes online. This address will change everytime you restart ngrok. Copy this appended with `/api/messages` to Messaging endpoint field.
        * If you restart ngrok you'll need to change this value at https://dev.botframework.com/bots
    - Click on 'Create Microsoft App ID and password. Then click on generate password. Copy your App ID and password and paste them in $LUCIDAROOT/botframework-interface/credentials.template
    - Rename credentials.template to credentials.js
    - Click on 'Register' to finish registering your bot.
* You may want to add channels on the bot page. Follow the instructions on https://dev.botframework.com/bots.

## Start Interface
* Change directory to $LUCIDAROOT/botframework-interface and run `make start_server`

## Add User
* Add the bot to your channel. This can be done using 'Add to Skype' button for Skype channel and adding yourself as developer for corresponding Facebook bot for Facebook Messenger channel.
* Connect your Lucida user to channel user
    - Sign in to the web interface of Lucida and click on your username.
    - Copy the verification message (Verify <token>) and send it using the channel you want to verify.

NOTE: The bot won't be available in bot directory unless you publish it. Till then only the people with 'Add to Skype' link (in case of Skype) and ones listed as developers/testers (in case of Facebook) will be able to send messages to the bot.
