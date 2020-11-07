const char MAIN_page[] PROGMEM = R"=====(

<html>
<head>
  <meta content="text/html;charset=utf-8" http-equiv="Content-Type">
  <meta content="utf-8" http-equiv="encoding">
  <meta name="viewport" content="width= device-width, initial-scale=1">
  <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
<script>
function show(shown, hidden1, hidden2, hidden3, hidden4) {
  document.getElementById(shown).style.display='block';
  document.getElementById(hidden1).style.display='none';
  document.getElementById(hidden2).style.display='none';
  document.getElementById(hidden3).style.display='none';
  document.getElementById(hidden4).style.display='none';
  return false;
}
</script>
<style>
  .button {
  border: none;
  color: white;
  padding: 10px 25px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 12px;
  margin: 3px 1px;
  cursor: pointer;
}

.Home_Button {background-color: #0B132B;} /* Green */
.MQTT_Button {background-color: #1C2541;} /* Blue */
.Status_Button {background-color: #3A506B;} /* Green */
.About_Button {background-color: #5BC0BE;} /* Blue */
.Control_Button{ background-color: #8F8F8F;}
</style>
</head>
<body>
  <a href="#" onclick="return show('Home Page','MQTT Page','Status Page','About Page','Control Page');"><button class="button Home_Button">HOME</button></a>
  <a href="#" onclick="return show('Control Page','Status Page','About Page','Home Page','MQTT Page');"><button class="button Control_Button">DATA</button></a>
  <a href="#" onclick="return show('MQTT Page','Home Page','Status Page','About Page','Control Page');"><button class="button MQTT_Button">SETTINGS</button></a> 
  <a href="#" onclick="return show('Status Page','About Page','Home Page','MQTT Page','Control Page');"><button class="button Status_Button">STATUS</button></a>
  <a href="#" onclick="return show('About Page','Status Page','MQTT Page','Home Page','Control Page');"><button class="button About_Button">ABOUT</button></a>  
  
  <div id="Home Page">
    <html>
    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
      <title>Tank monitor</title>
      <!--For offline ESP graphs see this tutorial https://circuits4you.com/2018/03/10/esp8266-jquery-and-ajax-web-server/ -->
      <script src = "https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.3/Chart.min.js"></script>  
      <style>
      canvas{
        -moz-user-select: none;
        -webkit-user-select: none;
        -ms-user-select: none;
      }

      
      html {
        font-family: 'Times New Roman', Times, serif;
        display: inline-block;
        margin: 0px auto;
      }
      </style>
    </head>

    <body>    
        <div style=" position: relative; text-align: center;width:100%;  font-size: 1.0rem">
          <p>
            <i class="fas fa-thermometer-half" style="color:#eb2906;"></i> 
            <span>Tank's Temperature</span> 
            <span id="tank_temp">%TEMPERATURE%</span>
            <sup>&deg;C</sup>
          </p>
          <p>
            <i class="fas fa-thermometer-half" style="color:#eb2906;"></i> 
            <span class=>Room's Temperature</span> 
            <span id="room_temp">%TEMPERATURE%</span>
            <sup>&deg;C</sup>
          </p>            
        </div>
        <div class="chart-container" style="position: relative; height:300px; width:100%;">
            <canvas id="Tank_Chart" width="100%" height="300"></canvas>
        </div>
        
        <div style="position: relative; text-align: center;width:100%; font-size: 1.0rem;">
        <p >
          <i class="fas fa-thermometer-half" style="color:#eb2906;"></i> 
          <span >Outdoor's Temperature</span> 
          <span id="outdoor_temp">%TEMPERATURE%</span>
          <sup>&deg;C</sup>
        </p>
        <p>
          <i class="fas fa-tint" style="color:#00add6;"></i> 
          <span class="dht-labels">Outdoor's Humidity</span>
          <span id="outdoor_humi">%HUMIDITY%</span>
          <sup>%</sup>
        </p>
      </div>
        <div class="chart-container" style="position: relative; height:300px; width:100%" >
            <canvas id="Outdoor_Chart" width="100%" height="300"></canvas>
        </div>
    <script>
    //Graphs visit: https://www.chartjs.org
        var Tank_temp = [];
        var Room_temp = [];
        var Outdoor_temp = [];
        var Outdoor_humi =[];
        var timeStamp = [];
        var wait_flag =new Boolean(false);
        var config = {
          type: 'line',
          data :{
            label : timeStamp,
            datasets: [{
              label: "Temperature",
              backgroundColor: 'rgba( 243, 156, 18 ,1)', //Dot marker color
              borderColor: 'rgba( 243, 156, 18 , 1)',
              fill: false,
              data : [0],
              pointRadius:0,
              },
              {
                label:"Humidity",
                backgroundColor: 'rgba(156, 18, 243 , 1)', //Dot marker color
                borderColor: 'rgba(156, 18, 243 , 1)', //Graph Line Color
                fill:false,
                data : [0],
                pointRadius:0,
            }],
          },
          options: {
                title: {
                        display: true,
                        text: "Out door Monitor"
                    },
                maintainAspectRatio: false,
                elements: {
                line: {
                        tension: 0.5 //Smoothening (Curved) of data lines
                    }
                },
                scales: {
                        yAxes: [{
                            ticks: {
                                beginAtZero:true
                            }
                        }]
                }
            }
        };
        var configForTankMonitor = {
          type: 'line',
          data :{
            label : timeStamp,
            datasets: [{
              label: "Tank's Temperature",
              backgroundColor:'rgba( 243, 156, 18 , 1)',
              borderColor: 'rgba( 243, 156, 18 , 1)',
              fill: false,
              data : [0],
              pointRadius:0,
              },
              {
                label:"Room's Temperature",
                backgroundColor: 'rgba(156, 18, 243 , 1)',
                borderColor: 'rgba(156, 18, 243 , 1)',
                fill:false,
                data : [0],
                pointRadius:0,
            }],
          },
          options: {
                title: {
                        display: true,
                        text: "Tank Monitor"
                    },
                maintainAspectRatio: false,
                elements: {
                line: {
                        tension: 0.5 //Smoothening (Curved) of data lines
                    }
                },
                scales: {
                        yAxes: [{
                            ticks: {
                                beginAtZero:true
                            }
                        }]
                }
            }
        };
    //On Page load show graphs
    window.onload = function() {
      console.log(new Date().toLocaleDateString()+" "+new Date().toLocaleTimeString());
      var ctx1 = document.getElementById("Tank_Chart").getContext('2d');
      window.myLineForTank = new Chart(ctx1,configForTankMonitor);

      var ctx = document.getElementById("Outdoor_Chart").getContext('2d');
      window.myLine = new Chart(ctx,config);
    };

    //Ajax script to get ADC voltage at every 5 Seconds 
    //Read This tutorial https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/

     setInterval(function() {
       // Call a function repetatively with 5 Second interval
       getData();
     }, 5000); //5000mSeconds update rate
    
    function getData() {
      wait_flag = false;
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        //Push the data in array
      var time = new Date().toLocaleDateString()+" "+new Date().toLocaleTimeString();
      var txt = this.responseText;
      var obj = JSON.parse(txt); //Ref: https://www.w3schools.com/js/js_json_parse.asp
          Tank_temp.push(obj.Tank_temp);
          Room_temp.push(obj.Room_temp);
          Outdoor_temp.push(obj.Outdoor_temp);
          Outdoor_humi.push(obj.Outdoor_humi);
          timeStamp.push(time);

      //Update for display value
      document.getElementById("outdoor_temp").innerHTML = obj.Outdoor_temp;
      document.getElementById("outdoor_humi").innerHTML = obj.Outdoor_humi;
      document.getElementById("tank_temp").innerHTML = obj.Tank_temp;
      document.getElementById("room_temp").innerHTML = obj.Room_temp;

      //Update for oudoor monitor graph
      config.data.labels.push(time);
      window.myLine.data.datasets[0].data.push(obj.Outdoor_temp);
      window.myLine.data.datasets[1].data.push(obj.Outdoor_humi);
      window.myLine.update();

      //Update for tank monitor graph
      configForTankMonitor.data.labels.push(time);
      window.myLineForTank.data.datasets[0].data.push(obj.Tank_temp);
      window.myLineForTank.data.datasets[1].data.push(obj.Room_temp);
      window.myLineForTank.update();
      var numberOfRows = document.getElementById("dataTable").rows.length;
      if (numberOfRows-1>=100){document.getElementById("dataTable").deleteRow(-1); }
      var table = document.getElementById("dataTable");
      var row = table.insertRow(1); 
      var cell0 = row.insertCell(0); 
      var cell1 = row.insertCell(1);
      var cell2 = row.insertCell(2);
      var cell3 = row.insertCell(3);
      var cell4 = row.insertCell(4);
      var cell5 = row.insertCell(5);
      cell0.innerHTML = numberOfRows-1;
      cell1.innerHTML = time;
      cell2.innerHTML = obj.Tank_temp;
      cell3.innerHTML = obj.Room_temp;
      cell4.innerHTML = obj.Outdoor_temp;
      cell5.innerHTML = obj.Outdoor_humi;
        }
      };
      xhttp.open("GET", "readADC", true);
      xhttp.send();
      wait_flag = true;
    }
        
    </script>
    </body>

    </html>
    
  </div>

  <div id="MQTT Page" style="display:none">
    <html>
        <style>
        input[type=text], select {
          width: 100%;
          padding: 12px 20px;
          margin: 8px 0;
          display: inline-block;
          border: 1px solid #ccc;
          border-radius: 4px;
          box-sizing: border-box;
        }

        input[type=password], select {
          width: 100%;
          padding: 12px 20px;
          margin: 8px 0;
          display: inline-block;
          border: 1px solid #ccc;
          border-radius: 4px;
          box-sizing: border-box;
        }

        input[type=submit] {
          width: 100%;
          background-color: #4CAF50;
          color: white;
          padding: 14px 20px;
          margin: 8px 0;
          border: none;
          border-radius: 4px;
          cursor: pointer;
        }

        input[type=submit]:hover {
          background-color: #45a049;
        }

        div {
          border-radius: 5px;
          background-color: #f2f2f2;
          padding: 20px;
        }
        </style>
        <body>
          <div style="position: absolute; top: 30%; left: 5%; width: 40%; ">
            <form action="/getMQTT" >
              <h1 style="text-align:center">MQTT CONFIG</h1>
              <label for="mqtt_broker">MQTT Broker</label>
              <input type="text" name="mqtt_broker">

              <label for="lname">Port</label>
              <input type="text" name="port">

              <label for="username">Username</label>
              <input type="text" name="username">
              
              <label for="password">Password</label>
              <input type="password"  name="password">

              <label for="topic">Topic</label>
              <input type="text" name="topic">
            </form><br>
            <button class="button button_submit_mqtt" onclick="MQTT_Config()" style="background-color:#365c5e;">SUBMIT</button>
            <p id="State"></p>
         </div>
          <script>
            function MQTT_Config(){
              while(!wait_flag);
              var xhttp =new XMLHttpRequest();
              xhttp.onreadystatechange = function(){
                if(this.readyState == 4 && this.status == 200){
                  document.getElementById("State").innerHTML = this.responseText;
                }
              };
              xhttp.open("GET", 
              "getMQTT?mqtt_broker="+document.getElementsByName('mqtt_broker')[0].value+"&port="+document.getElementsByName('port')[0].value+"&username="+document.getElementsByName('username')[0].value+"&password="+document.getElementsByName('password')[0].value+"&topic="+document.getElementsByName('topic')[0].value);
              xhttp.send();
            }
          </script>

          <div style="position: absolute; top: 30%; left: 55%; width: 40%; ">
            <form action="/getWiFi"  >
              <h1 style="text-align:center">WIFI SETTING</h1>
              <label for="lwifi">WIFI</label>
              <input type="text" name="wifi">

              <label for="lpassword">PASSWORD</label>
              <input type="password" name="wifi_password">

              <label for="lip">IP ADDRESS</label>
              <input type="text" name="ip_address">

              <label for="lgateway">GATEWAY</label>
              <input type="text" name="gateway">

              <label for="lsubnet">SUBNET</label>
              <input type="text" name="subnet">
            </form><br>
            <button class="button button_submit_wifi" onclick="WiFi_Config()" style="background-color: #365c5e;">SUBMIT</button>
            <p id="WiFiState"></p>
          </div>

          <script>
            function WiFi_Config(){
              while(!wait_flag);
              var xhttp =new XMLHttpRequest();
              xhttp.onreadystatechange = function(){
                if(this.readyState == 4 && this.status == 200){
                  document.getElementById("WiFiState").innerHTML = this.responseText;
                }
              };
              xhttp.open("GET", 
              "getWiFi?wifi="+document.getElementsByName('wifi')[0].value+"&password="+document.getElementsByName('wifi_password')[0].value+"&ip="+document.getElementsByName('ip_address')[0].value+"&gateway="+document.getElementsByName('gateway')[0].value+"&subnet="+document.getElementsByName('subnet')[0].value);
              xhttp.send();
            }
          </script>
        </body>
      </html>    
  </div>

  <!-- Information of the status of the system -->
  <div id="Status Page" style="display:none">
    <style>
      .status-label{
        font-size: 1.5rem;
        vertical-align:middle;
        padding-bottom: 15px;
      }
    </style>
    <h1 style="text-align:center" >STATUS OF THE DEVICE</h1>
    
    <h2>Sensor and SD Card State</h2>
    SD Card : <span id="SDCard">None</span><br>
    Sensor1: <span id="Sensor 1">None</span><br>
    Sensor2: <span id="Sensor 2">None</span><br>
    <h2>MQTT State</h2>
    MQTT : <span id="MQTT">None</span><br>
    JSON Format: <span id="JSON_Format">None</span><br>
    MQTT Broker: <span id="MQTT_broker">None</span><br>
    Port : <span id="Port">None</span><br>
    Topic: <span id="Topic">None</span><br>

    <h2>WiFi and IP data</h2>
    IP Address : <span id="IP">None</span><br>
    Gateway: <span id="gateway">None</span><br>
    WiFI connected: <span id="wifi_name">None</span><br>


    
    <button class="button" style="color:darkred; background-color: darkcyan;" onclick="updateDeviceState()">UPDATE</button></a>
    <button class="button" style="color:rgb(255, 255, 255); background-color: rgb(27, 37, 44); position: relative; left: 20%; top: 80%;" onclick="resetSetting()">RESET SETTING</button></a>
    <script>
      function updateDeviceState() {
        while(!wait_flag);
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        //Push the data in array
        var txt = this.responseText;
        var obj = JSON.parse(txt); //Ref: https://www.w3schools.com/js/js_json_parse.asp
          Tank_temp.push(obj.Tank_temp);
          Room_temp.push(obj.Room_temp);
          Outdoor_temp.push(obj.Outdoor_temp);
          Outdoor_humi.push(obj.Outdoor_humi);
        document.getElementById("MQTT").innerHTML = obj.MQTTState;
        document.getElementById("SDCard").innerHTML =obj.SDCardState;
        document.getElementById("Sensor 1").innerHTML = obj.Sensor1State;
        document.getElementById("Sensor 2").innerHTML =obj.Sensor2State;

        document.getElementById("MQTT_broker").innerHTML = obj.MQTT_Broker;
        document.getElementById("Port").innerHTML =obj.Port;
        document.getElementById("Topic").innerHTML = obj.Topic;
        document.getElementById("IP").innerHTML =obj.IP_address;
        document.getElementById("gateway").innerHTML =obj.gateway;
        document.getElementById("wifi_name").innerHTML =obj.wifi_name;

        if(obj.MQTTState=="Disconnected") document.getElementById("JSON_Format").innerHTML = "Hello";
        else document.getElementById("JSON_Format").innerHTML = '{"Tank_temp":"28.56", "Room_temp":"30.75", "Outdoor_temp":"29.30", "Outdoor_humi":"77.70"}';
        }
      };
      xhttp.open("GET", "deviceState", true);
      xhttp.send();
    }
    function resetSetting(){
      if(confirm("Reset setting")){
        while(!wait_flag);
        var xhttp =new XMLHttpRequest();
        xhttp.onreadystatechange = function(){
        if(this.readyState == 4 && this.status == 200){
          alert(this.responseText);         
        }
      };
      xhttp.open("GET", "resetSetting");
      xhttp.send();
      };
      
    }
    </script>
    
  </div>

  <!-- Information of the author -->
  <div id="About Page" style="display:none">
    <h1 style="text-align:center" >ABOUT</h1>
    <h2 style="text-align: center;" >Written by Kiet Tran</h2>
    <h2 style="text-align: center;">An Sinh Automation</h2>
      
  </div>

  <div id="Control Page" style="display:none">
    <style>
      .container{
        padding: 1rem;
        margin: 1rem;
      }
      .table-scroll{
        /*width:100%; */
        display: block;
        empty-cells: show;
        
        /* Decoration */
        border-spacing: 0;
        border: 1px solid;
        border-collapse: collapse;
      }

      .table-scroll thead{
        background-color: #f1f1f1;  
        position:relative;
        display: block;
        width:100%;
        overflow-y: scroll;
      }

      .table-scroll tbody{
        /* Position */
        display: block; position:relative;
        width:100%; overflow-y:scroll;
        /* Decoration */
        border-top: 1px solid rgba(0,0,0,0.2);
      }

      .table-scroll tr{
        width: 100%;
        display:flex;
      }

      .table-scroll td,.table-scroll th{
        flex-basis:100%;
        flex-grow:2;
        display: block;
        padding: 1rem;
        text-align:left;
      }

      /* Other options */

      .table-scroll.small-first-col td:first-child,
      .table-scroll.small-first-col th:first-child{
        flex-basis:15%;
        flex-grow:1;
      }

      .table-scroll tbody tr:nth-child(2n){
        background-color: rgba(130,130,170,0.1);
      }
      .body-half-screen{
        max-height: 70vh;
      }
      .small-col{flex-basis:10%;}
    </style>
        
        <table class="table-scroll small-first-col" id="dataTable">
          <thead>
            <tr>
              <th>ID</th>
              <th>Date and Time</th>
              <th>Tank's Temperature</th>
              <th>Room's Temperature (&deg;C)</th>
              <th>Outdoor Temperature (&deg;C)</th>
              <th>Outdoor Humidity (%)</th>
            </tr>
          </thead>
          <tbody class="body-half-screen" >
            <tr>
            </tr>
          </tbody>
        </table>
        <form action="/download" method="POST">
          <!-- <input id="file_name" style="width: 20%; height: 40px;" name="download" placeholder="File name....." value><br> -->
          <input type='submit' name="download" value='Download file' style="width: 30%;" onclick="Download_file()"></type='submit'>                 
        </form>
        
    </div>   
    </body>   
</html>
)=====";
