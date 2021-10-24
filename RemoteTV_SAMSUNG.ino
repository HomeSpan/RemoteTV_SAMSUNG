/* HomeSpan IR Remote TV Control */

#include "HomeSpan.h"
#include "extras/RFControl.h"

#define DEVICE_NAME   "SAMSUNG TV"

#define IR_PIN        10

#define SAMSUNG_ON    0xE0E09966
#define SAMSUNG_OFF   0xE0E019E6
#define SAMSUNG_HDMI1 0xE0E09768
#define SAMSUNG_HDMI2 0xE0E07D82
#define SAMSUNG_HDMI3 0xE0E043BC
#define SAMSUNG_HDMI4 0xE0E0A35C

RFControl rf(10);

void XMIT_SAMSUNG(uint32_t code){

  rf.enableCarrier(38000,0.5);

  int unit=563;

  rf.clear();

  rf.add(4500,4500);
  
  for(int i=31;i>=0;i--){
    rf.add(unit,unit*((code&(1<<i))?3:1));
    Serial.print((code&(1<<i))?1:0);
    if(!(i%8))
      Serial.print(" ");
  }

  rf.add(unit,45000);

  rf.start(2); 
}  

struct TV_Source : Service::InputSource{

  SpanCharacteristic *currentState = new Characteristic::CurrentVisibilityState(0,true);
  SpanCharacteristic *targetState = new Characteristic::TargetVisibilityState(0,true);

  TV_Source(uint32_t id, const char *name) : Service::InputSource(){
    new Characteristic::Identifier(id);
    new Characteristic::ConfiguredName(name,true);
    new Characteristic::IsConfigured(1);
  }

  boolean update() override{

    if(targetState->updated()){
      Serial.printf("Old Target State = %d    New Target State = %d\n",targetState->getVal(),targetState->getNewVal());
      currentState->setVal(targetState->getNewVal());
    }

    return(true);
  }
  
};

struct TV_Control : Service::Television {

  int nSources=0;
  vector<uint32_t> inputCodes;
  uint32_t onCode;
  uint32_t offCode;

  SpanCharacteristic *active = new Characteristic::Active(0,true);
  SpanCharacteristic *activeIdentifier = new Characteristic::ActiveIdentifier(1,true);

  TV_Control(const char *name, uint32_t onCode, uint32_t offCode) : Service::Television(){
    new Characteristic::ConfiguredName(name,true);
    this->onCode=onCode;
    this->offCode=offCode;
  }

  TV_Control *addSource(const char *name, uint32_t code){
    addLink(new TV_Source(nSources++,name));
    inputCodes.push_back(code);
    return(this);
  }
    
  boolean update() override{

    uint32_t code=0;

    if(active->updated()){
      code=active->getNewVal()?onCode:offCode;
      Serial.printf("Old Active State = %d    New Active State = %d\n",active->getVal(),active->getNewVal());
      Serial.printf("Code: %08X\n",code);
    }

    if(activeIdentifier->updated()){
      code=inputCodes[activeIdentifier->getNewVal()];
      Serial.printf("Old Active Identifier = %d    New Active Identifier = %d\n",activeIdentifier->getVal(),activeIdentifier->getNewVal());
      Serial.printf("Code: %08X\n",code);
    }

    if(code)
      XMIT_SAMSUNG(code);

    return(true);
  }

};

void setup() {

  Serial.begin(115200);
 
  homeSpan.begin(Category::Television,DEVICE_NAME);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("HomeSpan TV");
      new Characteristic::Manufacturer("HomeSpan");
      new Characteristic::SerialNumber("123-ABC");
      new Characteristic::Model("HomeSpan");
      new Characteristic::FirmwareRevision("0.1");
      new Characteristic::Identify();

  new Service::HAPProtocolInformation();
    new Characteristic::Version("1.1.0");
           
  (new TV_Control("My TV",SAMSUNG_ON,SAMSUNG_OFF))
    -> addSource("HDMI 1",SAMSUNG_HDMI1)
    -> addSource("HDMI 2",SAMSUNG_HDMI2)
    -> addSource("HDMI 3",SAMSUNG_HDMI3)
    -> addSource("HDMI 4",SAMSUNG_HDMI4)
    ;
}

void loop() {
  homeSpan.poll();
}
