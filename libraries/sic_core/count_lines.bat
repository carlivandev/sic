cd %~dp0\include
find . -name '*.h' | xargs wc -l
cd %~dp0\src
find . -name '*.cpp' | xargs wc -l
PAUSE