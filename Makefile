APPNAME = ChatServer
SRCPATH = ./src
BUILDPATH = ./build

APPNAME_W_PATH = $(BUILDPATH)/$(APPNAME)
CXXFLAGS =-std=c++11 -pthread -Wall -pedantic -Wextra -MD 

SRCOBJS := $(wildcard $(SRCPATH)/*.cpp)
OBJS := $(patsubst $(SRCPATH)/%.cpp,$(BUILDPATH)/%.o,$(SRCOBJS))

$(APPNAME_W_PATH):$(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) -lrt
    
$(BUILDPATH)/%.o : $(SRCPATH)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
      
.PHONY:clean    
clean:
	rm -f $(APPNAME_W_PATH) $(BUILDPATH)/*.o $(BUILDPATH)/*.d

-include $(OBJS:.o=.d)
