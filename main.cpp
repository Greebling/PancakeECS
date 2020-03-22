#include "src/ComponentView.h"
#include "src/GameObject.h"
#include "src/Scene.h"

// Declare new ComponentDatas
class TestData0 : public ComponentData
{
};

class TestData1 : public ComponentData
{

};


int main()
{
	// the scene we use
	Scene mainScene;
	
	// a view into the componentDatas we declared earlier
	ComponentView<TestData0, TestData1> testView;
	
	
	// create objects that hold components
	GameObject g0, g1, g2;
	g0.AddComponent<TestData0>();
	g0.AddComponent<TestData1>();
	
	g1.AddComponent<TestData0>();
	
	g2.AddComponent<TestData0>();
	g2.AddComponent<TestData1>();
	
	
	// iterate over the componentDatas we declared earlier
	testView.Foreach([&](TestData0 &data0, TestData1 &data1)
	                 {
		                 printf("Index is %i\n", data0.id.Index());
	                 });
	
	return 0;
}