var system = require('system')
var bfw_host = system.env["BFW_HOST"]
var username = system.env["BFW_UID"]
var password = system.env["BFW_PWD"]
var bothandle = system.env["BFW_HND"]
var botdata
var on_step = 0
var loading = false
var load_timeout = 0

if ( bfw_host == undefined || username == undefined || password == undefined ) {
  console.log("[ERROR] This script should be run from update_hosts.sh and not as a stand alone script!!!")
  phantom.exit(400)
}

if ( bfw_host.indexOf("https://") !== 0 ) {
  console.log("[ERROR] Only https addresses are allowed as endpoints!!!")
  phantom.exit(400)
}

if ( bfw_host.lastIndexOf("/api/messages") != (bfw_host.length - 13) ) {
  if ( bfw_host.lastIndexOf("/") != (bfw_host.length - 1) ) {
    bfw_host = bfw_host + "/"
  }
  bfw_host = bfw_host + "api/messages"
}

var page = require("webpage").create()
page.settings.userAgent = 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36'
page.settings.javascriptEnabled = true
page.settings.loadImages = false
phantom.cookiesEnabled = true
phantom.javascriptEnabled = true

phantom.onError = function(msg, trace) {
  var msgStack = ['[PHANTOM ERROR] ' + msg]
  if (trace && trace.length) {
    msgStack.push('TRACE:')
    trace.forEach(function(t) {
      msgStack.push(' -> ' + (t.file || t.sourceURL) + ': ' + t.line + (t.function ? ' (in function ' + t.function +')' : ''))
    })
  }
  console.error(msgStack.join('\n'))
  phantom.exit(503)
}

page.onLoadStarted = function() {
  loading = true
  clearTimeout(load_timeout)
}

page.onLoadFinished = function() {
  loading = false
}

page.onResourceRequested = function(requestData, networkRequest) {
  var match = requestData.url.match(/\.css/g)
  if (requestData.url == "https://dev.botframework.com/bots" ) {
    on_step = 3
    networkRequest.abort()
  }
}

var steps = [
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    console.log("[INFO] Connecting to https://login.microsoftonline.com...")
    page.open("https://login.microsoftonline.com/common/oauth2/authorize?response_type=id_token+code&redirect_uri=https%3A%2F%2Fdev.botframework.com%2F&client_id=abfa0a7c-a6b6-4736-8310-5855508787cd&resource=https%3A%2F%2Fmanagement.core.windows.net%2F&scope=user_impersonation+openid&nonce=8e87e9b7-db47-4187-08d4-9ce5cf4cd678&site_id=500879&response_mode=form_post&state=%2Flogin%3FrequestUrl%3D%252Fbots")
    return 0
  },
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    retval = page.evaluate(function(username, password) {
      userid = document.getElementById("cred_userid_inputtext")
      userpwd = document.getElementById("cred_password_inputtext")
      submit = document.getElementById("cred_sign_in_button")
      if ( userid == null || submit == null || userpwd == null ) { return 500 }
      userid.value = username
      userpwd.value = password
      submit.click()
      setTimeout(function(){document.getElementById("cred_sign_in_button").click()}, 10000)
      return 0
    }, username, password)
    if ( retval == 0 ) {
      console.log("[INFO] Verifying email address...")
    } else {
      console.log("[ERROR] Could not connect to Microsoft!!! Will try again in a second...")
    }
    return retval
  },
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    if ( page.url.indexOf("https://login.live.com") === 0 ) {
      retval = page.evaluate(function(password) {
        passwd = document.getElementById("i0118")
        submit = document.getElementsByTagName("form")[0]
        if ( passwd == null || submit == undefined ) { return 500 }
        passwd.value = password
        submit.submit()
        return 0
      }, password)
      if ( retval == 0 ) {
        console.log("[INFO] Logging into Microsoft Bot Framework...")
      } else {
        console.log("[ERROR] Could not connect to Microsoft!!! Will try again in a second...")
      }
      return retval
    } else {
      console.log("[ERROR] Your account or password is incorrect!!! Please re-enter your credentials...")
      return 401
    }
  },
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    if ( page.url.indexOf("https://account.live.com/identity/confirm") == 0 ) {
      console.log("[ERROR] Microsoft needs additional information to sign you in. This is probably because this server is in a different country than the one in which you usually use your Microsoft account. A workaround is to sign into your account from a browser via this server then run this script again.")
      return 403
    }
    console.log("[INFO] Trying to fetch bot data...")
    page.customHeaders = { "Accept": "application/json, text/javascript, */*; q=0.01" } /**/
    page.open("https://dev.botframework.com/api/BotManager/Bots?id=" + bothandle)
    return 0
  },
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    if ( page.url.indexOf("https://dev.botframework.com/api/BotManager/Bots") !== 0 ) {
      console.log("[ERROR] No bot with handle " + bothandle + " exists!!! Please enter a valid bot handle...")
      return 404
    }

    password=undefined
    var cookies = page.cookies
    for(var i in cookies) {
      if ( "Csrf-Token" == cookies[i].name ) { password = cookies[i].value }
    }
    if ( password == undefined ) {
      console.log("[ERROR] Your account or password is incorrect!!! Please re-enter your credentials...")
      return 401
    } else {
      console.log("[INFO] Logged in to Microsoft Bot Framework :D")
    }

    try {
      botdata = JSON.parse(page.plainText)
    } catch(err) {
      botdata = undefined
    }
    if ( botdata == undefined ) {
      console.log("[ERROR] Could not connect to Microsoft!!! Will try again in a second...")
      return 500
    }
    if ( botdata.Bot == undefined ) {
      console.log("[ERROR] No bot with handle " + bothandle + " exists!!! Please enter a valid bot handle...")
      return 404
    }
    botdata = botdata.Bot
    botdata.Endpoint = bfw_host
    for ( var endpoints in botdata.Endpoints ) { endpoints.Url = bfw_host }
    console.log("[INFO] Updating bot data...")
    settings = {
        operation: "PUT",
        headers: {
           "Content-Type":     "application/json",
           "Accept":           "application/json, text/javascript, */*; q=0.01",
           "Origin":           "https://dev.botframework.com",
           "X-Csrf-Token":     password,
           "X-Requested-With": "XMLHttpRequest"
        },
        data: JSON.stringify(botdata)
    }
    page.open("https://dev.botframework.com/api/BotManager?forPublishing=false", settings)
    return 0
  },
  function() {
    botdata=undefined
    try {
      botdata = JSON.parse(page.plainText)
    } catch(err) {
      botdata = undefined
    }
    if ( botdata == undefined ) {
      console.log("[ERROR] Could not connect to Microsoft!!! Will try again in a second...")
      return 500
    } else if ( botdata.Endpoint == undefined ) {
      console.log("[ERROR] Could not connect to Microsoft!!! Will try again in a second...")
      return 500
    }
    console.log("[INFO] Successfully updated bot endpoint :D")
    return 0
  }
]

setInterval( function() {
  if ( on_step == steps.length ) {
    phantom.exit()
  } else if ( !loading && typeof steps[on_step] == "function" ) {
    retval = steps[on_step]()
    if ( retval !== 0 ) { phantom.exit(retval) }
    on_step++
  } else if ( on_step < 0 ) {
    on_step++
  }
}, 100)
