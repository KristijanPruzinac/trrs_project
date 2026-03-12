#include "webserver.h"

// ── Shared GPS state ────────────────────────────────────────
volatile GpsData gpsData = {0, 0, 0, false};
SemaphoreHandle_t gpsMutex;

// ── Server internals ────────────────────────────────────────
static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static DynamicJsonDocument savedLocations(2048);
static SaveLocationCallback onSaveCallback = nullptr;

void onSaveFunction(float lat, float lng) {
  eeprom_data_t eeprom_data;
  eeprom_data.lat = lat;
  eeprom_data.lng = lng;

  // TODO: Implement?
  eeprom_data.year = 0;
  eeprom_data.month = 0;
  eeprom_data.day = 0;
  eeprom_data.hour = 0;
  eeprom_data.minute = 0;
  eeprom_data.second = 0;

  bool ok = eeprom_save_data(eeprom_data);
  if (ok) {
      Serial.println("EEPROM save success");
  }
  else {
      Serial.println("EEPROM save failed");
  }
}

#define MAX_LOCATIONS 1

// DNS server for captive portal
static DNSServer dnsServer;

// Add this function to handle captive portal
void setupCaptivePortal() {
    // Redirect all DNS requests to the ESP32's IP
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Add a handler for any HTTP request to redirect to your page
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
}

// ── Inline HTML page ────────────────────────────────────────
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GPS Tracker</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;min-height:100vh;display:flex;justify-content:center;align-items:center;padding:16px}
.card{background:#16213e;border-radius:12px;padding:24px;width:100%;max-width:420px;box-shadow:0 4px 24px rgba(0,0,0,.4)}
h1{text-align:center;font-size:1.4rem;margin-bottom:20px;color:#00d2ff}
.gps-display{background:#0f3460;border-radius:8px;padding:16px;margin-bottom:16px}
.gps-row{display:flex;justify-content:space-between;align-items:center;padding:8px 0}
.gps-row:not(:last-child){border-bottom:1px solid rgba(255,255,255,.1)}
.gps-label{font-size:.85rem;color:#999}
.gps-value{font-size:1.1rem;font-weight:600;font-family:'Courier New',monospace;color:#00d2ff}
.status{text-align:center;padding:8px;border-radius:6px;margin-bottom:16px;font-size:.85rem}
.status.ok{background:rgba(0,210,100,.15);color:#00d264}
.status.no{background:rgba(255,80,80,.15);color:#ff5050}
#saved-list{max-height:200px;overflow-y:auto;margin-bottom:16px}
.saved-item{background:#0f3460;border-radius:6px;padding:10px 12px;margin-bottom:6px;display:flex;justify-content:space-between;align-items:center;font-size:.85rem}
.saved-item .coords{font-family:'Courier New',monospace;color:#00d2ff}
.saved-item .time{color:#999;font-size:.75rem}
.btn{width:100%;padding:14px;border:none;border-radius:8px;font-size:1rem;font-weight:600;cursor:pointer;transition:opacity .2s}
.btn:active{opacity:.7}
.btn-save{background:linear-gradient(135deg,#00d2ff,#0080ff);color:#fff;margin-bottom:8px}
.btn-clear{background:rgba(255,80,80,.2);color:#ff5050}
.btn:disabled{opacity:.4;cursor:not-allowed}
</style>
</head>
<body>
<div class="card">
  <h1>GPS Tracker</h1>
  <div id="status" class="status no">Connecting...</div>
  <div class="gps-display">
    <div class="gps-row"><span class="gps-label">Latitude</span><span id="lat" class="gps-value">--</span></div>
    <div class="gps-row"><span class="gps-label">Longitude</span><span id="lng" class="gps-value">--</span></div>
    <div class="gps-row"><span class="gps-label">Satellites</span><span id="sat" class="gps-value">--</span></div>
  </div>
  <div id="saved-list"></div>
  <button class="btn btn-save" id="btn-save" disabled onclick="saveLocation()">Save Location</button>
</div>
<script>
var lat=0,lng=0,sat=0,fix=false;
function initEvents(){
  if(!window.EventSource){document.getElementById('status').textContent='Browser not supported';return;}
  var es=new EventSource('/events');
  es.addEventListener('gps',function(e){
    var d=JSON.parse(e.data);
    lat=d.lat;lng=d.lng;sat=d.sat;fix=d.fix;
    document.getElementById('lat').textContent=fix?lat.toFixed(6):'--';
    document.getElementById('lng').textContent=fix?lng.toFixed(6):'--';
    document.getElementById('sat').textContent=sat;
    var st=document.getElementById('status');
    if(fix){st.textContent='GPS Fix OK ('+sat+' sats)';st.className='status ok';}
    else{st.textContent='No GPS Fix ('+sat+' sats)';st.className='status no';}
    document.getElementById('btn-save').disabled=!fix;
  });
  es.onopen=function(){
    document.getElementById('status').textContent='Connected';
    document.getElementById('status').className='status ok';
  };
  es.onerror=function(){
    document.getElementById('status').textContent='Disconnected';
    document.getElementById('status').className='status no';
  };
}
function saveLocation(){
  if(!fix)return;
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({lat:lat,lng:lng})})
  .then(function(r){return r.json();}).then(function(d){if(d.ok)loadSaved();});
}
function loadSaved(){
  fetch('/locations').then(function(r){return r.json();}).then(function(arr){
    var el=document.getElementById('saved-list');el.innerHTML='';
    for(var i=arr.length-1;i>=0;i--){
      var item=arr[i];var div=document.createElement('div');div.className='saved-item';
      div.innerHTML='<span class="coords">'+item.lat.toFixed(6)+', '+item.lng.toFixed(6)+'</span><span class="time">#'+(i+1)+'</span>';
      el.appendChild(div);
    }
  });
}
initEvents();loadSaved();
</script>
</body>
</html>
)rawliteral";

void gps_webserver_topic_callback(dds_callback_context_t* context) {
    gps_data_t* data = (gps_data_t*)context->message_data.data;
    //IMPORTANT: Check HDOP value before using GPS data, it indicates the quality of the data
    webServerUpdateGps(data->lat,data->lng,data->satellites,data->hdop<10);
}

// ── Init: WiFi AP + routes + SSE ────────────────────────────
void webServerInit(const char *ssid, const char *password) {
  savedLocations.to<JsonArray>();
  gpsMutex = xSemaphoreCreateMutex();

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  setupCaptivePortal();

  onSaveCallback = onSaveFunction;

  // Serve inline HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
  });

  // GET saved locations
  server.on("/locations", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json;
    serializeJson(savedLocations, json);
    request->send(200, "application/json", json);
  });

  // POST save current GPS location
  server.on("/save", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<128> incoming;
      if (deserializeJson(incoming, data, len)) {
        request->send(400, "application/json", "{\"ok\":false}");
        return;
      }
      float lat = incoming["lat"];
      float lng = incoming["lng"];

      JsonArray arr = savedLocations.as<JsonArray>();
      if (arr.size() >= MAX_LOCATIONS) {
        arr.remove(0);
      }
      JsonObject obj = arr.createNestedObject();
      obj["lat"] = lat;
      obj["lng"] = lng;

      // Notify external code so it can persist to NVS
      if (onSaveCallback) {
        onSaveCallback(lat, lng);
      }

      request->send(200, "application/json", "{\"ok\":true}");
    }
  );

  // DELETE clear all locations
  server.on("/locations", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    savedLocations.clear();
    savedLocations.to<JsonArray>();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  // SSE
  events.onConnect([](AsyncEventSourceClient *client) {
    Serial.println("SSE client connected");
  });
  server.addHandler(&events);

  server.begin();
  Serial.println("Web server started");
}

// ── Register save callback ───────────────────────────────────
void webServerOnSave(SaveLocationCallback cb) {
  onSaveCallback = cb;
}

// ── Feed GPS data from external module ───────────────────────
void webServerUpdateGps(float lat, float lng, int sat, bool fix) {
  if (xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(50))) {
    gpsData.lat = lat;
    gpsData.lng = lng;
    gpsData.sat = sat;
    gpsData.fix = fix;
    xSemaphoreGive(gpsMutex);
  }
}

// ── SSE broadcast task (run on Core 0) ──────────────────────


static dds_thread_context_t thread_context;
static void thread_timer_callback(void* arg) { xTaskNotify(thread_context.task, THREAD_NOTIFY_BIT, eSetBits); }
void webServerTask(void* parameter) {
    thread_context.task = xTaskGetCurrentTaskHandle();
    thread_context.queue = xQueueCreate(5, sizeof(dds_callback_context_t));
    thread_context.sync_mutex = xSemaphoreCreateMutex();
    
    esp_timer_create_args_t timer_args = {
        .callback = &thread_timer_callback,
        .arg = NULL,
    };
    esp_timer_create(&timer_args, &(thread_context.timer));
    esp_timer_start_periodic(thread_context.timer, 10000); // 10 ms

    // ------- THREAD SETUP CODE START -------
    webServerInit(AP_SSID, NULL);
    dds_result_t result = DDS_SUBSCRIBE("/gps", gps_webserver_topic_callback, &thread_context);
    if (result != DDS_SUCCESS) {
        Serial.printf("Topic subscribe failed: %s\n", DDS_RESULT_TO_STRING(result));
    }
    // ------- THREAD SETUP CODE END -------

    vTaskDelay(500);
    
    while(1) {
        // Wait for any notification (message or timer)
        uint32_t notification_value;
        xTaskNotifyWait(0x00, 0xFF, &notification_value, portMAX_DELAY);
        
        if (notification_value & DDS_NOTIFY_BIT) { // DDS message notification
            DDS_TAKE_MUTEX(&thread_context);
            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
        if (notification_value & THREAD_NOTIFY_BIT) { // Timer tick notification
            DDS_TAKE_MUTEX(&thread_context);

            // ------- THREAD LOOP CODE START -------
            if (events.count() > 0) {
            GpsData data;
              if (xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(50))) {
                data = {gpsData.lat, gpsData.lng, gpsData.sat, gpsData.fix};
                xSemaphoreGive(gpsMutex);
              }

              char json[128];
              snprintf(json, sizeof(json),
                "{\"lat\":%.6f,\"lng\":%.6f,\"sat\":%d,\"fix\":%s}",
                data.lat, data.lng, data.sat, data.fix ? "true" : "false");

              events.send(json, "gps", millis());
            }

            dnsServer.processNextRequest();
            
            // ------- THREAD LOOP CODE END -------

            DDS_PROCESS_THREAD_MESSAGES(&thread_context);
            DDS_GIVE_MUTEX(&thread_context);
        }
    }
}