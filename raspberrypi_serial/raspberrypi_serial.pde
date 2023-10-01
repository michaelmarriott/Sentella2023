import mqtt.*;
import processing.sound.*;
import processing.net.*;
import processing.serial.*;
 
SoundFile file;
SoundFile file2;

Server s;
Client c;
String input;
int data[];
Timer timer;
Timer timerNext;
 
int numPorts=0;  // the number of serial ports in use
int maxPorts=24; // maximum number of serial ports
int currentPattern = 1;
int currentSpeed = 100;
int currentBright = 200;
int maxInstruction =8;
int patternLength[] = {240,240,240,240,240,240,240,240};
 
String mqqtServer = "127.0.0.1";//// "192.168.0.106";
String allowedSerials = "COM7"; // /dev/

Serial[] ledSerial = new Serial[maxPorts];     // each port's actual Serial port
 
int foundSerials = 0;
int errorCount=0;

MQTTClient mqttClient;
JSONObject json;

void setup() {
  size(800,800);
  background(255);
   file = new SoundFile(this,"ls.mp3");

  file.play(0.5,1);
  
  json = new JSONObject();
  mqttClient = new MQTTClient(this);
  mqttClient.connect("mqtt://"+mqqtServer, "processing");
  serialSetup();
  
  //this loads the file based on the file name
 
}


void playSound(){
  println("playSound:");
  //this loads the file based on the file name

    file2 = new SoundFile(this,"ls.mp3");
  file2.play(1.3,1);

}

void keyPressed() {
  //mqttClient.publish("/hello", "world");
  playSound();
}

void clientConnected() {
  println("client connected");
  mqttClient.subscribe("/sensor/#");
}

void messageReceived(String topic, byte[] payload) {
  //println("new message: " + topic + " - " + new String(payload));
  JSONObject json = parseJSONObject(new String(payload));
  if (json == null) {
    println("JSONObject could not be parsed");
  } else {
    ledWriteEvent(json);
    playSound();
  }
}

void connectionLost() {
  println("connection lost");
}
 
void serialSetup()
{
  String[] list = Serial.list();
  foundSerials = list.length;
  delay(20);
  println("Serial Ports List : ");
  println(Serial.list());
  if (errorCount > 0) exit();
  ///dev/serial1 /dev/ttyACM0 /dev/ttyAMA0
  for(String serial : Serial.list()){  
    if(serial.startsWith(allowedSerials)){
      serialConfigure(serial);
    }
  }
  //serialConfigure("/dev/ttyACM1");
  
  timer = new Timer(1);
  timerNext = new Timer(1);
  frameRate(5); // Slow it down a little
  s = new Server(this, 8080); // Start a simple server on a port
}
 
// movieEvent runs for each new frame of movie data
void ledWriteEvent(JSONObject data) {
 
  if(data !=  null) {
   // println(data.toString());
    String sensor =  json.getString("sensor");
    String distance =  json.getString("distance");

    //String ledData = "{"+padPattern +" "+padBright+" "+padSpeed+"}";
    String ledData = String.format("{\"sensor\":"+sensor+", \"distance\":+"+distance+"}");
    println(ledData.toString());
    println("numPorts:" + numPorts);
    for (int i=0; i < numPorts; i++) {  
      // send the raw data to the LEDs  <span class="Emoticon Emoticon1"><span>:-)</span></span>
      ledSerial[i].write(ledData);
    }
  }
}
 
// ask a Teensy board for its LED configuration, and set up the info for it.
void serialConfigure(String portName) {
  try {
    Serial newSerial = null;
    println("serialConfigure");
    try {
      newSerial = new Serial(this, portName,9600);
      println("delay over");
      //ledSerial[numPorts] = new Serial(this, portName,9600);
      //if (lnewSerial == null) throw new NullPointerException();
      newSerial.write('?');
    } catch (Throwable e) {
      println(e);
    //  println("Serial port " + portName + " does not exist or is non-functional");
      errorCount++;
      return;
    }
    println("delay");
    delay(500);
    println("delay over");
    String line = newSerial.readStringUntil(10);
    println("delay over");
    if (line == null) {
      newSerial.write('?');
      line = newSerial.readStringUntil(10);
      if (line == null) {
        println("Serial port " + portName + " is not responding.");
        println("Is it really a Teensy 3.2 running VideoDisplay?");
        errorCount++;
        return;
      }
    }
    println("here");
    String param[] = line.split(",");
    if (param.length != 12) {
      println("----"+line);
      println("Error: port " + portName + " did not respond to LED config query "+param.length);
      errorCount++;
      return;
    }
    ledSerial[numPorts]  = newSerial;
    println(ledSerial[numPorts]);
    println("----"+line);
    println("we got teensy on " + portName + "");
    numPorts++;
  }catch(Exception ex) {
     println("----"+ex);
  }
}
 
void draw()
{
  background(200);
  timer.countUp();
  timerNext.countUp();
  fill(0);
  float currentTime = timer.getTime();
  float currentNextTime = timerNext.getTime();
  if (keyPressed) {
    keyPressed();
  }
  if(currentTime > 1000 && 1 == 0){  
    // print(".");
    // println(currentNextTime);
    String[] list = Serial.list();
    if(foundSerials < list.length){
      for (int i=0; i < numPorts; i++) {  
        ledSerial[i].clear();
        ledSerial[i].stop();
      }
      numPorts =0;
    }
    if(foundSerials != list.length || numPorts == 0){
    //  println(list);
      println("Found new device");
      foundSerials = list.length;
      for(String serial : Serial.list()){
        if(serial.startsWith(allowedSerials)){
          serialConfigure(serial);
        }
      }
    }
    foundSerials = list.length;
    
    for (int i=0; i < numPorts; i++) {   
      // send the raw data to the LEDs  <span class="Emoticon Emoticon1"><span>:-)</span></span>
      println("---------");
      println(ledSerial[i]);
      println("---------");
    }
     
     // ledWriteEvent();
      timer.setTime(0);
  } 
 // println(currentPattern);
  if(currentPattern > 0 && currentPattern < patternLength.length && currentNextTime > patternLength[currentPattern-1]){
     println("PATTERN CHANGED");
     currentPattern+=1;
     if(currentPattern > maxInstruction){
       currentPattern = 1;
      // ledWriteEvent();
      }else{
     //  ledWriteEvent();
      }
      timerNext.setTime(0);
  }
 
  // Receive data from client
  c = s.available();
  if (c != null) {
    input = c.readString();
    input = input.substring(input.indexOf("{"), input.indexOf("}")+1); // Only up to the newline
    println("--??????--");
    data = int(split(input.substring(1,input.length()-1), ' ')); // Split values into an array
    println(input);
    currentPattern = data[0];
    currentBright = data[1];
    currentSpeed = data[2];
    timer.setTime(0);
    timerNext.setTime(0);
   // ledWriteEvent();
    // Draw line using received coords
   
    s.write(input.getBytes());
   // stroke(0);
   //line(data[0], data[1], data[2], data[3]);
    println("DISCONNECT");
    s.disconnect(c);
 
  }
 
}
