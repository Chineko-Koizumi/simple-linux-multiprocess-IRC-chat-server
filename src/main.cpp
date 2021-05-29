#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#include <iostream>
#include <algorithm>
#include <vector>

#define SEM_PATH "/main.cpp"
#define SEM_PATH_SHM "/chatlog.txt"

extern int errno ;

int client_socket;
int server_socket;

struct sockaddr_in server;
struct sockaddr_in client;

socklen_t client_addrlen;

const char * shm_name  	= "/AOS";
const int SIZE 			= 131072;
void* ptr;
int shm_fd;

sem_t* sem_shm;
sem_t*  sem_id;

bool DO = true; //send loop varible

struct data_info
{
	int size;
	int size_array_pid;
	
	int 	number_of_processes;
	int		flag_all_done;
}__attribute__((packed));

void send_SHM_msg_sig_handler(int signum)
{
	if( SIGUSR1 != signum) return;
	std::cout<<getpid()<<" before interupt\n";
	
	std::cout<<getpid()<<" inside interupt\n";
	
		struct data_info* shm_data;
		ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		shm_data = (data_info*)ptr;
		
		std::string temp_handler_str((char*)(ptr + sizeof(data_info) + shm_data->size_array_pid), shm_data->size);
		
		std::cout<<getpid()<<" before send\n";
		int send_return = send( client_socket, temp_handler_str.c_str(), temp_handler_str.size(), 0);
		
		if( 0 >= send_return){
			
			DO = false;
			shm_data->number_of_processes--;
			perror("send interupt msg for child process error");
			
		}
			
		shm_data->flag_all_done--;
	if( -1 == sem_post(sem_shm) ) perror("semaphore sig_handler error");
	
	std::cout<<getpid()<<" outside interupt send["<<send_return<<"]\n";
}

int main(int argc, char *argv[])
{
	int connetions_limit;
	int port_number;
	int max_connections;
		
	
	std::vector<pid_t> PID_vector;
	std::vector<pid_t>::iterator it;
	
	sem_id 			= sem_open( SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
	sem_shm			= sem_open( SEM_PATH_SHM, O_CREAT, S_IRUSR | S_IWUSR, 1);
	
			
	if( SEM_FAILED == sem_id){
		perror("Sem creation failed");
		return 0;
	}
	
		shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
		if ( -1 == shm_fd ){ 
			perror("SHM creation failed");
			return 0;
		}
		std::string shm_init_msg("INIT MSG FOR CHECKING PURPOSES\n"); 
		
		struct data_info init_data;
		init_data.size 					= shm_init_msg.size();
		init_data.size_array_pid		= 0;
		init_data.number_of_processes 	= 0;
		init_data.flag_all_done			= 0;
		
		ftruncate( shm_fd, sizeof(data_info) + shm_init_msg.size() );
		
		ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if( MAP_FAILED == ptr){ 
			perror("main mmap error");
			return 0;
		}
		
		memcpy( ptr, &init_data, sizeof(data_info) );
		memcpy( ptr + sizeof(data_info),  shm_init_msg.c_str(), shm_init_msg.size() );
		
	pid_t init = fork();
	
	if (init<0) exit(1); /* fork error */
	if (init>0) return 0; /* parent exits */
	
	if (argc!=3){
		 std::cout<<argv[0] <<" uses two arguments (port number and room size)\n";
		 return 0;
	}
	else{
		
		std::string str_port_number(argv[1]);
		port_number = stoi( str_port_number);
		
		std::string str_room_size(argv[2]);
		connetions_limit = stoi( str_room_size );
	
		max_connections = connetions_limit;
	
		std::cout<<"\nEntered port:["<<port_number<<"]\n";

		client_addrlen = sizeof(struct sockaddr_in);
	
		if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			perror("main.cpp Could not create socket");
			return 0;
		}
		else
		{
			std::cout<<"Socket created\n";
		}
		
		server.sin_family 		= AF_INET;
		server.sin_addr.s_addr 	= INADDR_ANY;
		server.sin_port 		= htons(port_number);
		
		if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == -1){
			
			perror("main.cpp Bind failed");
			return 0;
		}
		else{
			std::cout<<"Binding done\n";
		}
			
			if(listen(server_socket, max_connections)) perror("main.cpp listen error");
			
			else std::cout<<"Waiting for connetions... \n";
		
			int room_size = 0;
		
			sem_id 	= sem_open(SEM_PATH, 0);
			
			sigset_t signal_usr; 
			sigemptyset(&signal_usr);
			sigaddset(&signal_usr, SIGUSR1);
			sigprocmask(SIG_BLOCK, &signal_usr, NULL);
			
			while (true){
				usleep(100000);
				client_socket = accept(server_socket, (struct sockaddr *)&client, &client_addrlen);
				room_size++;
				if (client_socket == -1){
					
					perror("main.cpp accept failed");
					return 0;
				}
				else{
					std::cout<<"accepted connetion from:"<<inet_ntoa(client.sin_addr)<<"\n";
					
					pid_t PID1;
					fd_set read_fd;
					
					int sem_value;
					sem_getvalue(sem_id, &sem_value);
					
					std::cout<<getpid()<<" semaphore PID's reiit before value:"<<sem_value<<"\n";
					if( -1 == sem_wait(sem_id) ) perror("semaphore PID's reiit error");
					std::cout<<getpid()<<" semaphore PID's reiit inside\n";
					
					PID1 = fork();
					
					if ( 0 < PID1 ){
						int status;
						
						if( -1 == PID1){ 
							perror("fork creaiting child error");
							return 0;
						}
						else{ 
							PID_vector.push_back(PID1);
							
							for( unsigned int i=0; i<PID_vector.size(); ++i)
							{
								pid_t temp_pid = waitpid( PID_vector[i], &status, WNOHANG);
								if( -1 == temp_pid) perror("wait pid error");
								else if( 0 != temp_pid ){
									PID_vector.erase( PID_vector.begin() + i);
									room_size--;
									i--;
								}
							}
						
							
							ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
							data_info *resize_data_info = (data_info*)ptr;
							
							std::string temp_resize_string( (char*)( ptr + sizeof(data_info) + resize_data_info->size_array_pid ), resize_data_info->size );
							
							resize_data_info->size_array_pid = PID_vector.size() * sizeof(pid_t);
							
							ftruncate( shm_fd, sizeof(data_info) + resize_data_info->size_array_pid + resize_data_info->size );
							
							memcpy( ptr + sizeof(data_info), &PID_vector[0], resize_data_info->size_array_pid );
							memcpy( ptr + sizeof(data_info) + resize_data_info->size_array_pid, temp_resize_string.c_str(), resize_data_info->size );
							
							if( -1 == sem_post(sem_id) ) perror("semaphore PID's reiit error");
							std::cout<<getpid()<<" semaphore PID's reiit outside\n";
						}
						
						std::cout<<"\n+-------------------------------+\n";
					for( unsigned int i=0; i<PID_vector.size(); ++i) std::cout<<" ["<<PID_vector[i]<<"] ";
						std::cout<<"\n+-------------------------------+\n";
					}else{
							if(room_size > connetions_limit){
								std::string temp("room is full");
								if( -1 == send( client_socket, temp.c_str(), temp.size(), 0))perror("send init msg for grandchild process error");
								if ( shutdown( client_socket, SHUT_RDWR)) perror("main.cpp child process close socket error");
								if ( close( client_socket )) perror("main.cpp child process close socket error");
								return 0;
							}
						
							sem_id 		= sem_open(SEM_PATH, 0);	
							sem_shm  	= sem_open(SEM_PATH_SHM, 0);
							
							std::string init_client_msg = std::to_string(getpid()) + std::string(" Welcome to IRC \n\r");
							
							if( -1 == send( client_socket, init_client_msg.c_str(), init_client_msg.size(), 0))perror("send init msg for grandchild process error");
							
							std::cout<<getpid()<<" semaphore init msg send before\n";
							if( -1 == sem_wait(sem_id) ) perror("semaphore init msg send error");
							std::cout<<getpid()<<" semaphore init msg send inside\n";
							
								sigset_t signal_sigpipe; 
								sigemptyset(&signal_sigpipe);
								sigaddset(&signal_sigpipe, SIGPIPE);
								sigprocmask(SIG_BLOCK, &signal_sigpipe, NULL);
								
								if ( signal(SIGUSR1, send_SHM_msg_sig_handler) == SIG_ERR ) perror("can't catch SIGUSR\n");
								
								sigprocmask(SIG_UNBLOCK, &signal_usr, NULL);
							
								struct data_info* shm_data;
								shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
								ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
								shm_data = (data_info*)ptr;
								
								shm_data->number_of_processes++;
								
								std::string temp_check("Current PIDs: ");
								
								pid_t* tab = (pid_t*)(ptr + sizeof(data_info));
								for(unsigned int i=0; i<(shm_data->size_array_pid/sizeof(pid_t)); ++i)
								{
									temp_check += std::to_string(tab[i]) + std::string(" ");
								}
								temp_check += std::string("\n\r") + std::string("Enter your nickname: ");
								if( -1 == send( client_socket, temp_check.c_str(), temp_check.size(), 0))perror("send init msg for grandchild process error");
					
							if( -1 == sem_post(sem_id) ) perror("semaphore init msg send error");
							std::cout<<getpid()<<" semaphore init msg send outside\n";
							std::string nickname;
							
							bool flag_nickname_set 	= false;
							
							struct timeval tv;
							
							while(DO)
							{
							//std::cout<<getpid()<<" semaphore mail loop before\n";	
							if( -1 == sem_wait(sem_id) ) perror("semaphore mail loop error");
							//std::cout<<getpid()<<" semaphore mail loop inside\n";
								
								FD_ZERO( &read_fd );
								FD_SET( client_socket, &read_fd );
								
								tv.tv_sec = 0;
								tv.tv_usec = 50000;
								
								std::string temp_perror;
								
								switch( select( client_socket+ 1, & read_fd , NULL, NULL, &tv ) )
								{
									case - 1:
										temp_perror = std::to_string(getpid()) + std::string(" ") + std::string(" select error child process ");
										perror( temp_perror.c_str() );
									break;
									
									case 0:
										
									break;
										
									default:
										if( FD_ISSET( client_socket, & read_fd ) ){
											
										char static_buffor[2048];
										std::string temp;
										std::string temp_buff;
											
											std::cout<<getpid()<<" before recv\n";								
										int null_term = recv( client_socket, static_buffor, 2048,0 );
											std::cout<<getpid()<<" after recv recv null_term value:["<< null_term <<"]\n";	
										if( -1 == null_term )perror( "recv for child process error" );
										
										else if( 0 == null_term)temp = std::string("EXIT100");
										
										else{
											static_buffor[null_term] = '\0';
											temp_buff = std::string(static_buffor);
											temp = nickname + temp_buff;
										}
											
										if( std::string::npos != temp.find("EXIT100") ){ 
											DO 				= false;
											
											ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
											shm_data = (data_info*)ptr;
										
											if(flag_nickname_set) temp = nickname + std::string("connetion with client lost\n");
											else  temp = std::string("connetion with unknown client lost\n");
										
											shm_data->flag_all_done = shm_data->number_of_processes;
											shm_data->size 			= temp.size();
										
											ftruncate( shm_fd, sizeof(data_info) + shm_data->size_array_pid + temp.size() );
										
											memcpy( ptr + sizeof(data_info) + shm_data->size_array_pid, temp.c_str(), temp.size());
										
											if( -1 == kill( 0, SIGUSR1 ) ) perror("Kill error ");
											
											shm_data->number_of_processes--;
											
											break;
										}
										
										if(flag_nickname_set){
											
											ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
											shm_data = (data_info*)ptr;
											
											shm_data->flag_all_done = shm_data->number_of_processes;
											shm_data->size 			= temp.size();
											
											ftruncate( shm_fd, sizeof(data_info) + shm_data->size_array_pid + temp.size() );
											
											memcpy( ptr + sizeof(data_info) + shm_data->size_array_pid, temp.c_str(), temp.size());
											
											if( -1 == kill( 0, SIGUSR1 ) ) perror("Kill error ");
											
											std::cout<<getpid()<<" before sleep\n";
											while(true)
											{
												if( 0 >= shm_data->flag_all_done )break;
												usleep(10000);
											}
											std::cout<<getpid()<<" after sleep\n";
										}
										else
										{
											temp_buff.pop_back();
											temp_buff.pop_back();
											temp_buff.resize(16, ' ');
											nickname 			= temp_buff + std::string(": ");
											flag_nickname_set 	= true;
										}
										
									}	
									break;
								}
							
								if( -1 == sem_post(sem_id) ) perror("semaphore main loop error");
								//std::cout<<getpid()<<" semaphore mail loop outside nextloop\n";
								usleep(10000);
							}
								std::cout<<getpid()<<" semaphore mail loop perma exit outside\n";
								std::cout<<"connetion from: "<<inet_ntoa(client.sin_addr)<<" lost \n";
								FD_ZERO( &read_fd );
							if ( shutdown( client_socket, SHUT_RDWR)) perror("main.cpp child process close socket error");
							if ( close( client_socket )) perror("main.cpp child process close socket error");
							
							return 0;	
						}
					
						
					}
			}
	}

	return 0;
}

