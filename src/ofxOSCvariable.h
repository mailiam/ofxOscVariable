/*
 *  ofxOscVariable.h
 *  ofxOscVariableExample
 *
 *  Created by Sejun Jeong on 12. 7. 21.
 *  Copyright 2012년 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _ofxOscVariable
#define _ofxOscVariable

#include "ofMain.h"
#include "ofxOsc.h"

class ofxOscVariableInstanceBase {
public:
	
	string type;
	
	//just for dynamic casting
	virtual vector<string> getPaths(){
		return paths;
	}
	
	
	vector<string> paths;	
};

template <typename T>
class ofxOscVariableInstance : public ofxOscVariableInstanceBase {
public:
	void setInstance(T& var){
		instance = &var;
		type = typeid(*instance).name();
	}
	bool isNew(){
		return !(lastValue == *instance);
	}
	
	bool set(T value){
		lastValue = value;// *instance;
		*instance = value;
		return !(lastValue == value);
	}
	T get(){
		lastValue = *instance;
		return *instance;
	}
	T * instance;
	
	
private:
	T lastValue;

};

typedef ofxOscVariableInstance<int32_t> intOscVariable;
typedef ofxOscVariableInstance<uint64_t> int64OscVariable;
typedef ofxOscVariableInstance<float> floatOscVariable;
typedef ofxOscVariableInstance<string> stringOscVariable;


/////////////////////////////////////////////////////////////////////////////////


class ofxOscVariablePathBase {
public:
	virtual void send()=0;
	virtual void send(ofxOscSender& sender) = 0;
	virtual void receive(ofxOscMessage m){}
	
	string type;
	string path;
};

template <typename T>
class ofxOscVariablePath_ : public ofxOscVariablePathBase {
public:
	ofxOscVariablePath_(){
		type = typeid(T).name();
	}
	void addVariable(T& var){
		ofxOscVariableInstance<T> *oscVar = new ofxOscVariableInstance<T>;
		oscVar->setInstance(var);
		oscVar->paths.push_back(path);
		variables.push_back(oscVar);
	}
	
	virtual void send(){
		cout << "NOT IMPLEMENTED" <<endl;
	}
	
	virtual void send(ofxOscSender& sender){
		for(int i=0; i< variables.size(); i++){
			if(variables[i]->isNew()){
				ofxOscMessage m;
				m.setAddress(path);
				m.addIntArg(variables[i]->get());
				sender.sendMessage(m);
			}
		}
		
	}
	
	int getValueFromMessage(ofxOscMessage m, int type, int index = 0){ //type just for specialization
		return m.getArgAsInt32(index);
	}
	uint64_t getValueFromMessage(ofxOscMessage m, uint64_t type, int index = 0){ //type just for specialization
		return m.getArgAsInt64(index);
	}
	float getValueFromMessage(ofxOscMessage m, float type, int index = 0){ //type just for specialization
		return m.getArgAsFloat(index);
	}
	string getValueFromMessage(ofxOscMessage m, string type, int index = 0){ //type just for specialization
		return m.getArgAsString(index);
	}
	
	virtual void receive(ofxOscMessage m){
		T value = getValueFromMessage(m,(T)0);
		for(int i=0; i< variables.size(); i++){
			variables[i]->set(value);
		}
	}
	
	vector<ofxOscVariableInstance<T>*> variables;

private:

};


typedef ofxOscVariablePath_<int32_t> oscIntPath;
typedef ofxOscVariablePath_<uint64_t> oscInt64Path;
typedef ofxOscVariablePath_<float> oscFloatPath;
typedef ofxOscVariablePath_<string> oscStringPath;


////////////////////////////////////////////////////////////////////////////////////////


class ofxOscVariable : public ofThread {
	
  public:
	~ofxOscVariable(){
		stopThread();
	}
	
	ofxOscVariablePathBase* getPath(string path){
		for(int i=0; i<paths.size(); i++){
			if( paths[i]->path == path) return paths[i];
		}
		return false;
	}
/*
	oscIntPath* addIntPath(string path){
		if(getPath(path)) return dynamic_cast<oscIntPath *>(getPath(path));
		else{
			oscIntPath *oscPath = new oscIntPath;
			oscPath->path = path;
			paths.push_back(oscPath);
			return oscPath;
		}
	}
 */
	
	template <typename T>
	ofxOscVariablePath_<T>* addVariableToPath(T& var, string path){
		ofxOscVariablePath_<T> *oscPath;
		
		if(getPath(path))
		{
			ofLog(OF_LOG_VERBOSE, "The osc path already exists, appending to the path.");
			oscPath = dynamic_cast<ofxOscVariablePath_<T> *>(getPath(path));
		}else {
			oscPath = new ofxOscVariablePath_<T>;
			oscPath->path = path;
			paths.push_back(oscPath);
		}
		
		for(int i=0; i<oscPath->variables.size(); i++){
			if(oscPath->variables[i]->instance == &var){
				ofLog(OF_LOG_WARNING, "The variable is already assigned to the path.");
				return oscPath;
			}
		}
		
		oscPath->addVariable(var);
		
		return oscPath;
	}
	
	
	
	void threadedFunction(){
		while (isThreadRunning()){
			for (int i =0; i<paths.size(); i++){
				if(lock()){
					paths[i]->send(sender);
					unlock();
				}
			}
			
			while(receiver.hasWaitingMessages()){
				// get the next message
				ofxOscMessage m;
				receiver.getNextMessage(&m);
				// check the address of the incoming message
				for(int i=0; i<paths.size(); i++){
					if(m.getAddress() == paths[i]->path){
						paths[i]->receive(m);
					}
				}
				/*
				if(m.getAddress() == "/chatlog"){
					// get the first argument (we're only sending one) as a string
					if(m.getNumArgs() > 0){
						if(m.getArgType(0) == OFXOSC_TYPE_STRING){
							//string oldMessages = clientMessages;
							//clientMessages = m.getArgAsString(0) + "\n" + oldMessages;
						}
					}
				}*/
			}
			
		}
	}
	
	ofxOscReceiver receiver;
	ofxOscSender sender;
	
  private:
	
	vector<ofxOscVariablePathBase*> paths;
	
	vector< ofxOscVariableInstanceBase*> variables;
	
};

#endif
	