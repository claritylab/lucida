var system = require('system')
var username = system.env["BFW_UID"]
var password = system.env["BFW_PWD"]
var on_step = 0
var loading = false
var load_timeout = 0

if ( username == undefined || password == undefined ) {
  console.log("[ERROR] This script should be run from start_interface.sh and not as a stand alone script!!!")
  phantom.exit(400)
}

var page = require("webpage").create()
page.settings.userAgent = 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36'
page.settings.javascriptEnabled = true
page.settings.loadImages = false
page.settings.resourceTimeout = 60000
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

page.onResourceTimeout = function(request) {
    console.log('[ERROR] Request timed out while waiting for response... (#' + request.id + '): ' + JSON.stringify(request));
};

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
      console.log("[ERROR] The email address you entered is not associated with a Microsoft account!!! Please type a valid email address...")
      return 401
    }
  },
  function() {
    loading = true
    load_timeout = setTimeout(function(){ console.log("[ERROR] Request timed out while sending request..."); loading = false }, 20000)
    if ( page.url.indexOf("https://login.microsoftonline.com/login.srf") !== 0 && page.url.indexOf("https://login.microsoftonline.com/common/login") !== 0 ) {
      console.log("[ERROR] Your account or password is incorrect. Please re-enter your password...")
      return 401
    }
    console.log("[INFO] Logged into Micrsoft Account :D")
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
