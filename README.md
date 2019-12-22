# sic
very very wip!

sic is an engine where objects are pre-defined list of components and (s)ystems (i)terate (c)omponents to manipulate their state.

object type creation:
```cpp
struct Player : sic::Object<Player, Component_transform, Component_model> {};
```

component type creation:
```cpp
//model depends on transform, and will cache a pointer to it on creation,
//throws compile time error if object doesn't have a transform
struct Component_model : sic::Component<Component_transform>
{
	sic::Asset_ref<sic::Asset_model> m_model;
};
```

object creation:
```cpp
Player& player = context.create_object<Player>();
```

component iteration:
```cpp
context.for_each<Component_model>
(
	[](Component_model& instance)
	{
		//gets the cached dependency pointer
		instance.get<Component_transform>();
	}
);
```
