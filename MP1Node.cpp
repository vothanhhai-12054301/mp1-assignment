/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
	return 0;

}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    //kiem tra check 
    checkMessages();

    // loi group s
    if( !memberNode->inGroup ) {
    	return;
    }

    //responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;


    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	if (size <(int) sizeof(MessageHdr){
	#ifdef DEBUGLOG
		log->LOG(&memberNode->addr,"thong diep nhan nho hon MessageHdr");
	#endif
		return false;	
	}

	switch (msg ->msgType){
		case JOINREQ :
			return recvJoinReq(env,data + sizeof(MessageHdr),size -sizeof(MessageHdr));
	}
	return false;
}

bool MP1Node::recvJoinReq(void *env,char *data,int size){
	if(size < (int)(sizeof(memberNode ->addr.addr)+ sizeof(long))){

#ifdef DEBUGLOG
	log->LOG(&memberNode->addr,"loi JOINEQ nhan sai kich thuoc .");
#endif 
	return false;
	}

	Address joinaddr;
	long heartbeat;//tao ra heartbeat
	memcpy(joinaddr.addr,data,sizeof(memberNode->addr.addr));
	memcpy(&heartbeat, data + sizeof(memberNode->addr.addr), sizeof(long));

	int id = *(int*)(&joinaddr.addr);
	int port = *(short*)(&joinaddr.addr[4]);
	//viet ham them 
	updateMember(id, port, heartbeat); 
	sendMemberList("JOINREP", JOINREP, &joinaddr);
    return true;

}
void MP1Node::updateMember(int id, short port, long heartbeat)
{
    vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
    for (; it != memberNode->memberList.end(); ++it) {
        if (it->id == id && it->port ==port) {
            if (heartbeat > it->heartbeat) {
                it->setheartbeat(heartbeat);
                it->settimestamp(par->getcurrtime());
            }
            return;
        }
    }
    //them member List Entry
    MemberListEntry memberEntry(id,port,heartbeat,par->getcurrtime());
    memberNode->memberList.push_back(memberEntry);
#ifdef DEBUGLOG
    Address joinaddr;
    memcpy(&joinaddr.addr[0],&id,sizeof(int));
    memcpy(&joinaddr.addr[4],&port,sizeof(short));
    log->logNodeAdd(&memberNode->addr,&joinaddr);
#endif

}

void MP1Node::updateMember(MemberListEntry& member)
{
    updateMember(member.getid(), member.getport(), member.getheartbeat());
}

int MP1Node::memcpyMemberListEntry(char * data,MemberListEntry& member)
{
	char * p =data ;
	memcpy(p,&member.id,sizeof(int));
	p + = sizeof(int);
	memcpy(p , &member.port, sizeof(short));
    p += sizeof(short);
    memcpy(p , &member.heartbeat, sizeof(long));
    p += sizeof(long);
    return p - data;
}
}
void MP1Node::sendMemberList(const char * label, enum MsgTypes msgType, Address * to)
{
    long members = memberNode->memberList.size();
    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + members * (sizeof(int) + sizeof(short) + sizeof(log));


    MessageHdr * msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    char * data = (char*)(msg + 1);

    msg->msgType = msgType;
    memcpy(data, &memberNode->addr.addr, sizeof(memberNode->addr.addr));
    data += sizeof(memberNode->addr.addr);
    char * pos_members = data;
    data += sizeof(long);

    for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin() ; it != memberNode->memberList.end();) {

        if (it != memberNode->memberList.begin()) {

            if (par->getcurrtime() - it->timestamp > TREMOVE) {
                // Member CLEANUP!
#ifdef DEBUGLOG
                Address joinaddr;
                memcpy(&joinaddr.addr[0], &it->id, sizeof(int));
                memcpy(&joinaddr.addr[4], &it->port, sizeof(short));
                log->logNodeRemove(&memberNode->addr, &joinaddr);
#endif
                members--;
                it = memberNode->memberList.erase(it);
                continue;
            }

            if (par->getcurrtime() - it->timestamp > TFAIL) {

                members--;
                ++it;
                continue;
            }
        }

        //logMemberListEntry(*it);

        data += memcpyMemberListEntry(data, *it);
        ++it;
    }

    memcpy(pos_members, &members, sizeof(long));
    msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + members * (sizeof(int) + sizeof(short) + sizeof(log));

    emulNet->ENsend(&memberNode->addr, to, (char *)msg, msgsize);
    free(msg);
}
/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */

     if (par->getcurrtime() > 3 && memberNode->memberList.size() > 1) {

        memberNode->memberList.begin()->heartbeat++;
        memberNode->memberList.begin()->timestamp = par->getcurrtime();

        int pos = rand() % (memberNode->memberList.size() - 1) + 1;
        MemberListEntry& member = memberNode->memberList[pos];

        if (par->getcurrtime() - member.timestamp > TFAIL) {
            return;
        }

        Address memberAddr;
        memcpy(&memberAddr.addr[0], &member.id, sizeof(int));
        memcpy(&memberAddr.addr[4], &member.port, sizeof(short));

        sendMemberList("HEARTBEATREQ", HEARTBEATREQ, &memberAddr);
    }
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();

    int id = *(int*)(&memberNode->addr.addr);
    int port = *(short*)(&memberNode->addr.addr[4]);
    MemberListEntry memberEntry(id, port, 0, par->getcurrtime());
    memberNode->memberList.push_back(memberEntry);
    memberNode->myPos = memberNode->memberList.begin();
}
