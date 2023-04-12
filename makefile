cc=g++
PARSER=parser
SERVER=Serach_Server

.PHONY:all
all:$(PARSER) $(SERVER)

$(SERVER):Search_Server.cc
	$(cc) $^ -o $@ -std=c++11 -ljsoncpp -lpthread -lboost_system -lboost_filesystem  -lboost_thread
$(PARSER):parser.cc
	$(cc) $^ -o $@ -std=c++11 -lboost_system -lboost_filesystem  -lboost_thread
.PHONY:clean
clean:
	rm -f $(PARSER) $(SERVER)