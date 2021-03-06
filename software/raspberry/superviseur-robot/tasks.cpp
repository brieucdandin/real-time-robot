/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSENDTOMON 24
#define PRIORITY_TSTARTSTOPCAM 21
#define PRIORITY_TSTARTROBOT 21
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TSENDARENA 19
#define PRIORITY_TMOVE 10
#define PRIORITY_TBATTERY 8
#define PRIORITY_TSENDIMAGE 5

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 *
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 *
 * 3- Data flow is probably not optimal
 *
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 *
 * 5- Same behavior exists for ComMonitor::Write !
 *
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 *
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex,
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_cameraStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_cameraToBeStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_send, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_image, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_withWD, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startCamera, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComCamera, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_findArena, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_watchDog, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_battery, "th_battery", 0, PRIORITY_TBATTERY, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startStopCam, "th_startStopCam", 0, PRIORITY_TSTARTSTOPCAM, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendArena, "th_sendArena", 0, PRIORITY_TSENDARENA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendImage, "th_sendImage", 0, PRIORITY_TSENDIMAGE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_ReloadWD, "th_ReloadWD", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_battery, (void(*)(void*)) & Tasks::GetBatteryLevel, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startStopCam, (void(*)(void*)) & Tasks::StartStopCam, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendImage, (void(*)(void*)) & Tasks::SendImage, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendArena, (void(*)(void*)) & Tasks::SendArena, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_ReloadWD, (void(*)(void*)) & Tasks::ReloadWD, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
            monitor.Close(); 
            rt_mutex_release(&mutex_monitor);
            cout << "communication with monitor lost " << endl;
            delete(msgRcv);
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            watchdog = false; 
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {
            watchdog = true;
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } else if(msgRcv->CompareID(MESSAGE_CAM_OPEN)){
            rt_sem_v(&sem_startCamera);
        } else if(msgRcv->CompareID(MESSAGE_CAM_CLOSE)){
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            camera.Close();
            rt_mutex_release(&mutex_camera);
            WriteInQueue(&q_messageToMon, new Message(MESSAGE_ANSWER_ACK));
        }else if (msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA)){
            rt_sem_v(sem_findArena);
        }
               
        
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        bool WD ;
        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        
        rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
        WD = watchdog;
        rt_mutex_release(&mutex_withWD);
        if (WD){
            cout << "Start robot with watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithWD());
            rt_mutex_release(&mutex_robot);
            rt_sem_v(&sem_watchDog);
       } else {
            cout << "Start robot without watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithoutWD());
            rt_mutex_release(&mutex_robot);
        }
        
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);

            cout << " move: " << cpMove;

            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            robot.Write(new Message((MessageID)cpMove));
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
    }
}

/**
 * @brief Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}


/*
 *
 * =============== OUR TASKS ===============
 *
 */

// =============== MONITOR PART ===============

/**
 * @brief Every 500 ms, asks robot for battery level and sends it to monitor.
 *
 * @param void
 * @return void
 */
void Tasks::GetBatteryLevel() {
    Message* battery_level;
    int rs;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    rt_task_set_periodic(NULL, TM_NOW, 500000000);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/

    while(1) {
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            // A message to ask for battery level update is created and sent to the robot.
            battery_level = robot.Write(robot.GetBattery());
            rt_mutex_release(&mutex_robot);
            // The answer is printed on the monitor.
            WriteInQueue(&q_messageToMon, battery_level);
        }
        // Wait for period
        rt_task_wait_period(NULL);
    }
}

void Tasks::ReloadWD(void *arg) {

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    rt_sem_p(&sem_watchDog, TM_INFINITE);
    
    bool WD = false;
    int cpt = 0;
    Message *msgSend;
     
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);
    
    while (1){
        rt_task_wait_period(NULL);         
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        int rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        
        if (rs == 1) {
            rt_mutex_acquire(&mutex_withWD, TM_INFINITE);
            WD = watchdog;
            rt_mutex_release(&mutex_withWD);            
            if (WD) {
                  rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                  Message *msg_reload = robot.Write(robot.ReloadWD());
                  rt_mutex_release(&mutex_robot);                  
                  if(msg_reload->GetID() == MESSAGE_ANSWER_ACK) {cpt = 0;}    
                  else {cpt++;}  
                }
            
            if (cpt > 3){
                rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                robotStarted = 0;
                rt_mutex_release(&mutex_robotStarted);
                
                rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                robot.Close();
                rt_mutex_release(&mutex_robot);
                
                rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
                msgSend = new Message(MESSAGE_ANSWER_COM_ERROR);
                WriteInQueue(&q_messageToMon, msgSend);                
                rt_mutex_release(&mutex_monitor);
                
                rt_sem_v(&sem_openComRobot);
            }
        }
    }   
}



// =============== CAMERA PART ===============

/**TODO: Test
 * @brief Starts the camera; then wait for instruction from monitor to switch it off.
 *
 * @param void
 * @return void
 * @note MESSAGE_ANSWER_ACK is send to the monitor only once the camera has been effectively switched ON/OFF.
 */
void Tasks::StartStopCam() {
    Message* msgSend = new Message();

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    // Every quarter of second
    rt_task_set_periodic(NULL, TM_NOW, 250000000);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/

    while(1) {
        rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
        rt_mutex_acquire(&mutex_cameraToBeStarted, TM_INFINITE);
        // Start camera
        if(!camera_status_effective && camera_status_wanted) {
            rt_mutex_release(&mutex_cameraStarted);
            rt_mutex_release(&mutex_cameraToBeStarted);
            cout << "Start camera..." << endl << flush;
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            camera_status_effective = camera.Open();
            rt_mutex_release(&mutex_cameraStarted);
            cout << " Camera started." << endl << flush;
            // Starting retrieving images periodically
            rt_mutex_acquire(&mutex_send, TM_INFINITE);
            send_image = true;
            rt_mutex_release(&mutex_send);
            // Notifying monitor about camera status
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            if(camera_status_effective) {
                rt_mutex_release(&mutex_cameraStarted);
                msgSend = new Message(MESSAGE_ANSWER_ACK);
            } else {
                msgSend = new Message(MESSAGE_ANSWER_NACK);
            }
            cout << "Notifying monitor (" << msgSend->GetID() << ")." << flush;
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
            rt_mutex_release(&mutex_cameraStarted);
            cout << " Monitor notified." << endl << flush;
        }

        // Stop camera
        else if(camera_status_effective && !camera_status_wanted) {
            rt_mutex_release(&mutex_cameraStarted);
            rt_mutex_release(&mutex_cameraToBeStarted);
            // Stopping retrieving images periodically
            rt_mutex_acquire(&mutex_send, TM_INFINITE);
            send_image = false;
            rt_mutex_release(&mutex_send);
            // Stopping the camera
            cout << "Stopping camera..." << flush;
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            camera.Close();
            rt_mutex_release(&mutex_camera);
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            camera_status_effective = -camera.IsOpen();
            rt_mutex_release(&mutex_cameraStarted);
            cout << " Camera stopped." << endl << flush;
            // Notifying monitor about camera status
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            if(!camera_status_effective) {
                rt_mutex_release(&mutex_cameraStarted);
                msgSend = new Message(MESSAGE_ANSWER_ACK);
            } else {
                msgSend = new Message(MESSAGE_ANSWER_NACK);
            }
            cout << "Notifying monitor (" << msgSend->GetID() << ")." << flush;
            rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
            WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
            rt_mutex_release(&mutex_cameraStarted);
            cout << " Monitor notified." << endl << flush;
        }

        // When the camera is already in the state asked for, the supervisor sends back ACK.
        else if(camera_status_effective == camera_status_wanted) {
            rt_mutex_release(&mutex_cameraStarted);
            rt_mutex_release(&mutex_cameraToBeStarted);
            cout << "Camera is already in the desired state (" << camera_status_effective << ")." << endl << flush;
            Message* msgSend = new Message(MESSAGE_ANSWER_ACK);
            WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
            cout << " Monitor notified." << endl << flush;
        }

        // Else: communication error
        else {
            cout << "Communication error: received message that is not MESSAGE_CAM_OPEN nor MESSAGE_CAM_CLOSE." << flush;
            Message* msgSend = new Message(MESSAGE_ANSWER_COM_ERROR);
            WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
            cout << " Monitor notified." << endl << flush;
        }
        // Wait for period
        rt_task_wait_period(NULL);
    }
}

/**TODO: Test
 * @brief Periodicaly retrieves snapshots from the camera and places it in a public memory space.
 *
 * @param void
 * @return void
 */
void Tasks::SendImage() {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    // 4 images per second
    rt_task_set_periodic(NULL, TM_NOW, 250000000);

    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/

    cout << "Start sending images." << endl << flush;
    while (1) {
        rt_mutex_acquire(&mutex_send, TM_INFINITE);
        if(send_image) {
            rt_mutex_release(&mutex_send);
            if(!camera.IsOpen()) {
                camera_status_effective = false;
            } else {
                // Grab an image
                cout << "Ask for image..." << flush;
                rt_mutex_acquire(&mutex_image, TM_INFINITE);
                img = camera.Grab().Copy();
                rt_mutex_release(&mutex_image);
                cout << " Image received." << endl << flush;
              }
        }
        // Wait for period
        rt_task_wait_period(NULL);
    }
}

/**TODO: Test & validation of arena by monitor
 * @brief Locates the arena on an image and sends it to the monitor.
 *
 * @param void
 * @return void
 */
void Tasks::SendArena() {
    Arena arena;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task FindArena starts here                                                     */
    /**************************************************************************************/

    while(1) {
        rt_mutex_acquire(&mutex_cameraStarted, TM_INFINITE);
        rt_mutex_acquire(&mutex_send, TM_INFINITE);
        if(camera_status_effective && send_image) {
            rt_mutex_release(&mutex_cameraStarted);
            rt_mutex_release(&mutex_send);
            // Stopping retrieving images periodically
            rt_mutex_acquire(&mutex_send, TM_INFINITE);
            send_image = false;
            rt_mutex_release(&mutex_send);
            // Grab an image
            cout << "Ask for image..." << flush;
            rt_mutex_acquire(&mutex_image, TM_INFINITE);
            img = camera.Grab().Copy();
            rt_mutex_release(&mutex_image);
            cout << " Image received." << endl << flush;
            // Look for arena
            arena = img->SearchArena();
            // If arena is found
            if (!arena.IsEmpty()) {
                // Draw arena on the image and send it to the monitor
                img->DrawArena(arena);
                cout << "Arena drawn on image." << endl << flush;
                Message* msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
                WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
                cout << "Image with arena sent to Monitor." << endl << flush;
            // If arena is not found
            } else {
                cout << "Could not find arena." << endl << flush;
                Message* msgSend = new Message(MESSAGE_ANSWER_NACK);
                WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
                cout << " Monitor notified." << endl << flush;
            }
            // Starting retrieving images periodically again
            rt_mutex_acquire(&mutex_send, TM_INFINITE);
            send_image = true;
            rt_mutex_release(&mutex_send);
        } else {
            cout << "Camera off or not sending images periodically." << endl << flush;
            msg = new Message(MESSAGE_ANSWER_NACK);
            WriteInQueue(&q_messageToMon, msg); // msgSend will be deleted by sendToMon
            cout << " Monitor notified." << endl << flush;
        }
    }
}
