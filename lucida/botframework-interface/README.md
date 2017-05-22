Microsoft BotFramework interface to LUCIDA
==========================================

This interaface allows accessing LUCIDA services from BotFramework supported channels like Facebook, Skype, messaging etc. Currently only text infer queries are supported.

## Installation
* Change directory to $LUCIDAROOT/botframework-interface and run `make all`
* Create a new bot at https://dev.botframework.com/bots/new
    - Fill in the name, bot handle and description for your bot.
    - Click on 'Create Microsoft App ID and password. Then click on generate password. Copy your App ID and password and keep them in a safe place.
    - Leave all other fields blank unless you know what you are doing.
    - Agree to the terms and click on 'Register' to finish registering your bot.
* You may want to add channels on the bot page. Follow the instructions on https://dev.botframework.com/bots.

## Start Interface
* Change directory to $LUCIDAROOT/botframework-interface and run `make start_server`
* First run will ask some questions. These can me modified later by editing 'config.sh'.

## Add User
* Add the bot to your channel. This can be done using 'Add to Skype' button for Skype channel and adding yourself as developer for corresponding Facebook bot for Facebook Messenger channel.
* Connect your Lucida user to channel user
    - Sign in to the web interface of Lucida and click on your username.
    - Copy the verification message (Verify <token>) and send it using the channel you want to verify.

NOTE: The bot won't be available in bot directory unless you publish it. Till then only the people with 'Add to Skype' link (in case of Skype) and ones listed as developers/testers (in case of Facebook) will be able to send messages to the bot.
