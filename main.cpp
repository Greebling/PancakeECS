#include "src/ComponentView.h"
#include "src/GameObject.h"
#include "src/Scene.h"

class TestData0 : public ComponentData
{
};

class TestData1 : public ComponentData
{

};

class TestData2 : public ComponentData
{

};

int main()
{
	Scene mainScene;
	
	ComponentView<TestData0> testView0;
	ComponentView<TestData0, TestData1> testView;
	ComponentView<TestData0, TestData2> testView2;
	
	GameObject g0, g1, g2;
	
	g0.AddComponent<TestData0>();
	g0.AddComponent<TestData1>();
	g0.AddComponent<TestData2>();
	
	g1.AddComponent<TestData0>();
	
	g2.AddComponent<TestData0>();
	g2.AddComponent<TestData1>();
	g2.AddComponent<TestData2>();
	
	
	
	testView0.Foreach([&](TestData0 &data0)
	                  {
		                 printf("Index is %i\n", data0.id.Index());
	                  });
	
	return 0;
}