#make file
 
#compiler
CC=gcc
#build directory, store compile files here
BUILD_DIR=build
#src directory
SRC_DIR=src


TS=telnet_server
TC=telnet_client

  
### Clean :
cleanall:
			rm $(BUILD_DIR)/*.* $(BUILD_DIR)/*

### Compile :
installts:    
	    		$(CC) $(SRC_DIR)/$(TS).c -o $(BUILD_DIR)/$(TS)

installtc:
			$(CC) $(SRC_DIR)/$(TC).c -o $(BUILD_DIR)/$(TC)
	     
### Run :
runts:
			$(BUILD_DIR)/$(TS)

runtc:
			$(BUILD_DIR)/$(TC)

### Deploy :
deployts:
			sshpass -p "1995" scp /home/rss/Workspace/C-Language/Projects/telnet-chat-server/$(BUILD_DIR)/$(TS) rss@192.168.1.3:/home/rss/code/$(TS)/

### Remote Run :
remoterunts:
			plink rss@192.168.1.3 -pw 1995 /home/rss/code/$(TS)/$(TS)& 
