Source = $(wildcard *.c)
Objs = $(Source:%.c=%.o)  
CFLAGS += -Wall -g  
Target = treader 

all : $(Target)
$(Target):$(Objs)
	$(CC) -o $(Target) $(Objs)

.PHONY:clean
clean :
	-$(RM) $(Target) $(Objs)  
