// source: https://github.com/huangzehao/SimpleNeuralNetwork/
// author: huangzehao

#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

struct Connection
{
	double weight;
	double deltaWeight;
};

class Neuron;

typedef vector<Neuron> Layer;

// ****************** class Neuron ******************

class Neuron
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	void setOutputVal(double val) { m_outputVal = val; }
	double getOutputVal(void) const { return m_outputVal; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(double targetVals);
	void calcHiddenGradients(const Layer &nextLayer);
	void updateInputWeights(Layer &prevLayer);
	double m_gradient;

private:
	static double eta; // [0.0...1.0] overall net training rate
	static double alpha; // [0.0...n] multiplier of last weight change [momentum]
	static double transferFunction(double x);
	static double transferFunctionDerivative(double x);
	// randomWeight: 0 - 1
	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	double sumDOW(const Layer &nextLayer) const;
	double m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
};


// ****************** class Net ******************
class Net
{
public:
	Net(const vector<unsigned> &topology);
	void feedForward(const vector<double> &inputVals);
	void backProp(const vector<double> &targetVals);
	void getResults(vector<double> &resultVals) const;
	double getRecentAverageError(void) const { return m_recentAverageError; }

private:
	vector<Layer> m_layers; //m_layers[layerNum][neuronNum]
	double m_error;
	double m_recentAverageError;
	static double m_recentAverageSmoothingFactor;
};

// int main()
// {
// 	TrainingData trainData("trainingData.txt");
// 	//e.g., {3, 2, 1 }
// 	vector<unsigned> topology;
// 	//topology.push_back(3);
// 	//topology.push_back(2);
// 	//topology.push_back(1);

// 	trainData.getTopology(topology);
// 	Net myNet(topology);

// 	vector<double> inputVals, targetVals, resultVals;
// 	int trainingPass = 0;
// 	while(!trainData.isEof())
// 	{
// 		++trainingPass;
// 		cout << endl << "Pass" << trainingPass;

// 		// Get new input data and feed it forward:
// 		if(trainData.getNextInputs(inputVals) != topology[0])
// 			break;
// 		showVectorVals(": Inputs :", inputVals);
// 		myNet.feedForward(inputVals);

// 		// Collect the net's actual results:
// 		myNet.getResults(resultVals);
// 		showVectorVals("Outputs:", resultVals);

// 		// Train the net what the outputs should have been:
// 		trainData.getTargetOutputs(targetVals);
// 		showVectorVals("Targets:", targetVals);
// 		assert(targetVals.size() == topology.back());

// 		myNet.backProp(targetVals);

// 		// Report how well the training is working, average over recnet
// 		cout << "Net recent average error: "
// 		     << myNet.getRecentAverageError() << endl;
// 	}

// 	cout << endl << "Done" << endl;
// }

#endif