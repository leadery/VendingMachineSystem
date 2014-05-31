CC1=g++
CC2=gcc

CFLAGS1=-g -Wall -std=c++0x
CFLAGS2=-g -Wall

LDLIBS1=-lcrypto
LDLIBS2=-lpthread

GROUPNAME=LLL

ASSIGNMENT=project

FILES=program1.c program1_util.c program1_util.h program1_test.txt\
	program2.c program2_util.c program2_util.h user_info.txt \
	program3.c program3_util.c program3_util.h ini_server.txt\
	common.c common.h\
	program1_test.out.expected program2_test.out.expected program3_test.out.expected\
	program1_unittest.out.expected program2_unittest.out.expected program3_unittest.out.expected\
	presentation.pptx LLL_API_Documentation.pdf LLL_Usage_Instruction.pdf Makefile

AUTONAME=$(GROUPNAME)_$(ASSIGNMENT)

all:	program1 program2 program3
program1:	program1.c program1_util.c program1_util.h common.c common.h
	$(CC2) $(CFLAGS2) $^ $(LDLIBS1) -o $@ $(LDLIBS2)
program2:	program2.c program2_util.c program2_util.h common.c common.h
	$(CC1) $(CFLAGS1) $^ $(LDLIBS1) -o $@
program3:	program3.c program3_util.c program3_util.h common.c common.h
	$(CC2) $(CFLAGS2) $^ -o $@
test1:	program1 program2 program3
	# test LOGIN command
	# 1) test input format of userid(user id should consist of 8 characters)
	# 2) test input password format(length should be less than 16)
	# 3) test whether the userid and password matches or not
	# test LOGOUT command
	# test  DEPOSIT command
	# 1) test the input format
	# 2) test the functionality of DEPOSIT
	# test BALANCE command
	# test BUY command
	# 1) test when the balance is less than the price
	# 2) test input format
	# 3) test its functionality
	# test PRICE command
	# 1) when the index is out of range
	# 2) test its functionality
	# test EXIT command
	# Warning: because when the printf function takes more time to execute
	# so it is possible the real output is different from our expected one.
	# "INSUFFICIENT BALANCE" might be disordered in the standard output
	./program2 10689 127.0.0.1 20689 & 
	./program3 20689 127.0.0.1 30689 < ini_server.txt & 
	./program1 127.0.0.1 10689 30689 < program1_test.txt > program1_test.out
	killall ./program2
	killall ./program3
	diff program1_test.out.expected program1_test.out
	

test2:	program1 program2 program3
	# test when the userid and password pair is invalid
	# test when the user balance is insufficient for the purchase
	# test how the authentication center react to DEPOSIT/BALANCE command
	# test the correctness of forward functionality of authentication center
	./program2 10689 127.0.0.1 20689 > program2_test.out > program2_test.out& 
	./program3 20689 127.0.0.1 30689 < ini_server.txt > program3_test.out& 
	./program1 127.0.0.1 10689 30689 < program1_test.txt
	killall ./program2
	killall ./program3
	diff program2_test.out.expected program2_test.out

test3:	
	# program3 has been executed in test2. Here only run
	# diff to compare the output of program3 with its expected output
	# test how the vending server react to PRICE/BUY command
	# test how the vending server deal with insufficient balance
	# test how the vending server deal with invalid item index
	diff program3_test.out.expected program3_test.out

unittest1:	program1
	./program1 test > program1_unittest.out
	diff program1_unittest.out.expected program1_unittest.out
unittest2:	program2
	./program2 test > program2_unittest.out
	diff program2_unittest.out.expected program2_unittest.out
unittest3:	program3
	./program3 test > program3_unittest.out
	diff program3_unittest.out.expected program3_unittest.out


clean:
	rm -f program1 program2 program3

dist: builddist distcheck

builddist:
	a2ps -A fill -1 --header="${AUTONAME}" --line-numbers=1 -o ${AUTONAME}_code.ps ${FILES}
	ps2pdf ${AUTONAME}_code.ps
	-rm -rf ${GROUPNAME}/
	mkdir ${GROUPNAME}
	cp ${AUTONAME}_code.pdf ${GROUPNAME}/
	cp ${GROUPNAME}_${ASSIGNMENT}_report.pdf ${GROUPNAME}/
	cp ${FILES} ${GROUPNAME}/
	tar -cvzf ${AUTONAME}.tar.gz ${GROUPNAME}
	rm -rf ${GROUPNAME}/
	rm ${AUTONAME}_code.ps

distcheck:
	-rm -rf ${GROUPNAME}/
	tar -xvzf ${AUTONAME}.tar.gz
	@echo Checking if a PDF of your code, your main hwk.c file,
	@echo and your report PDF are included...
	@test -s ${GROUPNAME}/${AUTONAME}_code.pdf
