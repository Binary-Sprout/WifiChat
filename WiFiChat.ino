#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

const char* ssid = "WifiChat 1.0";
const char* password = ""; // Set a secure password if you want this private

AsyncWebServer server(80);
DNSServer dnsServer;

const int maxMessages = 10;
struct Message {
  String sender;
  String content;
  String ip;
};
Message messages[maxMessages];
int messageIndex = 0;
int currentMessageCount = 0; // Tracks the actual number of messages

std::vector<String> blockedIPs;

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body {
    background-color: #2F4F4F;
    font-family: Arial, sans-serif;
    margin: 0 auto;
    padding: 0;
    max-width: 500px;
    height: 100vh; /* Make sure the body fills the height of the viewport */
    display: grid;
    grid-template-rows: 1fr auto; /* Create two rows: content and form */
  }
  
  h2 {
    color: #fff;
    text-align: center;
  }
  
  .warning {
    color: #555;
    border-radius: 10px;
    font-family: Tahoma, Geneva, Arial, sans-serif;
    font-size: 12px;
    padding: 10px;
    margin: 10px;
    background: #fff8c4;
    border: 1px solid #f2c779;
    text-align: center;
  }

  #chat-container {
    display: flex;
    flex-direction: column;
    overflow-y: auto;
  }

  form {
    display: flex;
    flex-direction: column;
    padding: 10px;
    background-color: #2F4F4F;
    position: sticky;
    bottom: 0;
    width: 100%;
    box-sizing: border-box;
  }

  input[type=text], input[type=submit] {
    width: calc(100% - 22px);
    padding: 10px;
    margin: 10px 0;
    box-sizing: border-box;
    border-radius: 10px;
    border: 0px;
  }

  input[type=submit] {
    background: #7079f0;
    color: white;
    font-size: 17px;
    font-weight: 500;
    border-radius: 0.9em;
    border: none;
    cursor: pointer;
  }

  ul {
    list-style-type: none;
    padding: 0;
    margin: 0;
    max-width: 100%;
    flex: 1;
    overflow-y: auto;
    background-color: #e6e6e6;
  }

  li {
    background-color: #f9f9f9;
    margin: 5px 0;
    padding: 10px;
    border-radius: 5px;
    word-wrap: break-word;
  }

  #deviceCount {
    color: #fff;
    margin: 10px;
  }

  .link {
    color: #007BFF;
    text-decoration: none;
  }

  .link:hover {
    text-decoration: underline;
  }

  /* Make sure form and list don't overflow the screen */
  form input[type=text] {
    margin-bottom: 10px;
  }
</style>
<script>
function fetchData() {
  fetch('/messages')
    .then(response => response.json())
    .then(data => {
      const ul = document.getElementById('messageList');
      // Reverse the messages to show the latest at the top
      ul.innerHTML = data.messages.reverse().map(msg => `<li>${msg.sender}: ${msg.message}</li>`).join('');
    });
  updateDeviceCount();
}

function updateDeviceCount() {
  fetch('/deviceCount').then(response => response.json()).then(data => {
    document.getElementById('deviceCount').textContent = 'Users Online: ' + data.count;
  });
}

function saveName(name) {
  localStorage.setItem('userName', name);
}

function loadName() {
  let savedName = localStorage.getItem('userName');
  if (!savedName) {
    savedName = prompt("Please enter your name:");
    if (savedName) {
      saveName(savedName);
    } else {
      savedName = "Anonymous";  // Default if user cancels
    }
  }
  document.getElementById('nameInput').value = savedName;
}

window.onload = function() {
  loadName();
  fetchData();
  setInterval(fetchData, 5000);
  setInterval(updateDeviceCount, 5000);
};
</script>
</head>
<body>
<h2>WiFiChat 1.0</h2>
<div class="warning">WARNING! For your safety, do not share your location or any personal information!</div>
<div id="chat-container">
  <div id="deviceCount">Users Online: 0</div>
  <ul id="messageList"></ul>
</div>

<form action="/update" method="POST">
  <input type="text" id="nameInput" name="sender" placeholder="Enter your name" required oninput="saveName()" />
  <input type="text" name="msg" placeholder="Enter your message" required />
  <input type="submit" value="Send" />
</form>

<p style="text-align: center;">github.com/djcasper1975</p>
</body>
</html>
)rawliteral";

const char blockedPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      background-color: #f8f9fa;
      font-family: 'Arial', sans-serif;
      text-align: center;
      color: #333;
    }
    .container {
      background: #ffffff;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
      max-width: 90%;
      width: 400px;
    }
    h1 {
      font-size: 24px;
      margin-bottom: 10px;
      color: #d9534f;
    }
    p {
      font-size: 16px;
      margin-bottom: 20px;
    }
    .btn {
      display: inline-block;
      padding: 10px 20px;
      font-size: 16px;
      color: #fff;
      background-color: #007bff;
      border: none;
      border-radius: 5px;
      text-decoration: none;
      cursor: pointer;
      transition: background-color 0.3s;
    }
    .btn:hover {
      background-color: #0056b3;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Access Denied</h1>
    <p>Your access has been blocked due to a policy violation.</p>
    <p>If you believe this is a mistake, please contact the administrator.</p>
    <a href="/" class="btn">Return to Home</a>
  </div>
</body>
</html>
)rawliteral";

const char devicesPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { font-family: Arial, sans-serif; margin: 0; padding: 0; text-align: center; }
  h2 { color: #333; }
  ul { list-style-type: none; padding: 0; margin: 20px auto; max-width: 500px; }
  li { background-color: #f9f9f9; margin: 5px 0; padding: 10px; border-radius: 5px; word-wrap: break-word; overflow-wrap: break-word; white-space: pre-wrap; display: flex; flex-direction: column; align-items: center; }
  .button-container { display: flex; gap: 10px; margin-top: 10px; }
  button { padding: 5px 10px; background-color: #f44336; color: white; border: none; border-radius: 5px; cursor: pointer; }
  button:hover { background-color: #d32f2f; }
  .block-button { background-color: #f44336; }
  .unblock-button { background-color: #4CAF50; }
</style>
<script>
function fetchDeviceData() {
  fetch('/messages').then(response => response.json()).then(data => {
    const ul = document.getElementById('deviceList');
    ul.innerHTML = data.messages.map((msg, index) => 
      `<li>${msg.sender} (${msg.ip}): ${msg.message}
      <div class="button-container">
        <button onclick="deleteMessage(${index})" class="block-button">Delete</button>
        <button onclick="blockDevice('${msg.ip}')" class="block-button">Block</button>
      </div></li>`).join('');
  });
  fetchBlockedIPs();
  updateDeviceCount();
}

function fetchBlockedIPs() {
  fetch('/blockedIPs').then(response => response.json()).then(data => {
    const blockedList = document.getElementById('blockedList');
    blockedList.innerHTML = data.blockedIPs.map(ip => 
      `<li>${ip} <button onclick="unblockDevice('${ip}')" class="unblock-button">Unblock</button></li>`
    ).join('');
  });
}

function deleteMessage(index) {
  fetch(`/deleteMessage?index=${index}`, { method: 'GET' })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      fetchDeviceData();  // Refresh the list after deletion
    } else {
      alert('Failed to delete message: ' + data.error);
    }
  });
}

function blockDevice(ip) {
  fetch(`/blockDevice?ip=${ip}`, { method: 'GET' })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      alert('Device blocked');
      fetchDeviceData();  // Refresh the list after blocking
    } else {
      alert('Failed to block device');
    }
  });
}

function unblockDevice(ip) {
  fetch(`/unblockDevice?ip=${ip}`, { method: 'GET' })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      alert('Device unblocked');
      fetchDeviceData();  // Refresh the list after unblocking
    } else {
      alert('Failed to unblock device');
    }
  });
}

function updateDeviceCount() {
  fetch('/deviceCount').then(response => response.json()).then(data => {
    document.getElementById('deviceCount').textContent = 'Users Online: ' + data.count;
  });
}

window.onload = function() {
  fetchDeviceData();
  setInterval(fetchDeviceData, 5000);
  setInterval(updateDeviceCount, 5000);
};
</script>
</head>
<body>
<h2>Connected Devices and Messages</h2>
<ul id='deviceList'></ul>
<h2>Blocked IPs</h2>
<ul id='blockedList'></ul>
<p><a href="/">Back to Chat</a></p>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");

  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", htmlPage);
  });

  server.on("/devices", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", devicesPage);
  });

  server.on("/messages", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    for (int i = 0; i < currentMessageCount; i++) {
      if (!messages[i].content.isEmpty()) {
        if (i > 0) json += ",";
        json += "{\"sender\":\"" + messages[i].sender + "\",\"message\":\"" + messages[i].content + "\",\"ip\":\"" + messages[i].ip + "\"}";
      }
    }
    json += "]";
    request->send(200, "application/json", "{\"messages\":" + json + "}");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    String newMessage, senderName;
    if (request->hasParam("msg", true) && request->hasParam("sender", true)) {
      newMessage = request->getParam("msg", true)->value();
      senderName = request->getParam("sender", true)->value();
      String clientIP = request->client()->remoteIP().toString();

      // Check if the IP is blocked
      if (std::find(blockedIPs.begin(), blockedIPs.end(), clientIP) != blockedIPs.end()) {
        request->send_P(403, "text/html", blockedPage);
        return;
      }

      // Properly add the new message
      messages[messageIndex].content = newMessage;
      messages[messageIndex].sender = senderName;
      messages[messageIndex].ip = clientIP;

      // Update indices correctly
      messageIndex = (messageIndex + 1) % maxMessages;
      if (currentMessageCount < maxMessages) currentMessageCount++;
    }
    request->redirect("/");
  });

  server.on("/deleteMessage", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("index")) {
      int index = request->getParam("index")->value().toInt();
      if (index >= 0 && index < currentMessageCount) {
        // Correctly delete the message and adjust array
        for (int i = index; i < currentMessageCount - 1; i++) {
          messages[i] = messages[i + 1];
        }
        messages[currentMessageCount - 1] = {"", "", ""}; // Clear the last message slot
        currentMessageCount--;
        if (messageIndex == 0) {
          messageIndex = currentMessageCount; // Adjust messageIndex when at zero
        } else {
          messageIndex--;
        }
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(200, "application/json", "{\"success\":false, \"error\":\"Invalid index.\"}");
      }
    } else {
      request->send(200, "application/json", "{\"success\":false, \"error\":\"Index parameter missing.\"}");
    }
  });

  server.on("/blockDevice", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ip")) {
      String ip = request->getParam("ip")->value();
      if (std::find(blockedIPs.begin(), blockedIPs.end(), ip) == blockedIPs.end()) {
        blockedIPs.push_back(ip); // Block the IP address if not already blocked
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(200, "application/json", "{\"success\":false, \"error\":\"IP already blocked.\"}");
      }
    } else {
      request->send(200, "application/json", "{\"success\":false, \"error\":\"IP parameter missing.\"}");
    }
  });

  server.on("/unblockDevice", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("ip")) {
      String ip = request->getParam("ip")->value();
      auto it = std::find(blockedIPs.begin(), blockedIPs.end(), ip);
      if (it != blockedIPs.end()) {
        blockedIPs.erase(it); // Unblock the IP address
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(200, "application/json", "{\"success\":false}");
      }
    } else {
      request->send(200, "application/json", "{\"success\":false}");
    }
  });

  server.on("/blockedIPs", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    for (size_t i = 0; i < blockedIPs.size(); i++) {
      if (i > 0) json += ",";
      json += "\"" + blockedIPs[i] + "\"";
    }
    json += "]";
    request->send(200, "application/json", "{\"blockedIPs\":" + json + "}");
  });

  server.on("/deviceCount", HTTP_GET, [](AsyncWebServerRequest *request){
    int deviceCount = WiFi.softAPgetStationNum();
    request->send(200, "application/json", "{\"count\":" + String(deviceCount) + "}");
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
}
