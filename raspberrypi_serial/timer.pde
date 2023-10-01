class Timer{
  float time;
  
  Timer(float set){
    time = set;
  }
  float getTime(){
    return time;
  }
  void setTime(float set)
  {
    time = set;
  }
  void countUp(){
   time += 1/frameRate; 
  }
}
