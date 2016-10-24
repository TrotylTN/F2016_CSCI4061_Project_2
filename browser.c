/* CSci4061 F2016 Assignment 2
 * login: zhou0745
 * date: 10/28/16
 * name: Tiannan Zhou, Annelies Odermann, Lidiya Dergacheva
 * id: 5232494(zhou0745), 4740784(oderm008), 4515805 (derg0004) */
#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TAB 100

extern int errno;

/*
 * Name:				uri_entered_cb
 * Input arguments:	 'entry'-address bar where the url was entered
 *					  'data'-auxiliary data sent along with the event
 * Output arguments:	none
 * Function:			When the user hits the enter after entering the url
 *					  in the address bar, 'activate' event is generated
 *					  for the Widget Entry, for which 'uri_entered_cb'
 *					  callback is called. Controller-tab captures this event
 *					  and sends the browsing request to the ROUTER (parent)
 *					  process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data) {
	if(data == NULL) {
		return;
	}
	browser_window *b_window = (browser_window *)data;
	// This channel has pipes to communicate with ROUTER.
	comm_channel channel = b_window->channel;
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);
	if(tab_index <= 0 || tab_index >= MAX_TAB) {
		fprintf(stderr, "Invalid tab index (%d).\n", tab_index);
		return;
	}
	// Get the URL.
	char * uri = get_entered_uri(entry);
	// Append your code here
	// ------------------------------------
	// * Prepare a NEW_URI_ENTERED packet to send to ROUTER (parent) process.
	// * Send the url and tab index to ROUTER
	child_req_to_parent uriReq;
	uriReq.type = NEW_URI_ENTERED;
	uriReq.req.uri_req.render_in_tab = tab_index;
	strncpy(uriReq.req.uri_req.uri, uri, sizeof(uriReq.req.uri_req.uri));
	write(channel.child_to_parent_fd[1], &uriReq, sizeof(child_req_to_parent));
	return;
	// ------------------------------------
}

/*
 * Name:				create_new_tab_cb
 * Input arguments:	 'button' - whose click generated this callback
 *					  'data' - auxillary data passed along for handling
 *					  this event.
 * Output arguments:	none
 * Function:			This is the callback function for the 'create_new_tab'
 *					  event which is generated when the user clicks the '+'
 *					  button in the controller-tab. The controller-tab
 *					  redirects the request to the ROUTER (parent) process
 *					  which then creates a new child process for creating
 *					  and managing this new tab.
 */
void create_new_tab_cb(GtkButton *button, gpointer data) {
	if(data == NULL) {
		return;
	}
	// This channel has pipes to communicate with ROUTER.
	comm_channel channel = ((browser_window*)data)->channel;
	// Append your code here.
	// ------------------------------------
	// * Send a CREATE_TAB message to ROUTER (parent) process.
	child_req_to_parent tabReq;
	tabReq.type = CREATE_TAB;
	write(channel.child_to_parent_fd[1], &tabReq, sizeof(child_req_to_parent));
	// ------------------------------------
	return;
}

/*
 * Name:				url_rendering_process
 * Input arguments:	 'tab_index': URL-RENDERING tab index
 *					  'channel': Includes pipes to communctaion with
 *					  Router process
 * Output arguments:	none
 * Function:			This function will make a URL-RENDRERING tab Note.
 *					  You need to use below functions to handle tab event.
 *					  1. process_all_gtk_events();
 *					  2. process_single_gtk_event();
*/
int url_rendering_process(int tab_index, comm_channel *channel, int parent_pid) {
	// Don't forget to close pipe fds which are unused by this process
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create url-rendering window
	create_browser(URL_RENDERING_TAB, tab_index, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	// child_req_to_parent req;
	int flags, nread;
	flags = fcntl(channel->parent_to_child_fd[0], F_GETFL, 0);
	fcntl(channel->parent_to_child_fd[0], F_SETFL, flags | O_NONBLOCK);
	child_req_to_parent tabRcv;
	while (1) {
		if (kill(parent_pid, 0) != 0) {
			printf("Tab %d Error: Detected Router process's unexpected exit. Quit.\n", tab_index);
			process_all_gtk_events();
			free(channel);
			return 0;
		}
		// Handle one gtk event, you don't need to change it nor understand what it does.
		process_single_gtk_event();
		// Poll message from ROUTER
		// It is unnecessary to poll requests unstoppably, that will consume too much CPU time
		// Sleep some time, e.g. 1 ms, and render CPU to other processes
		usleep(1000);
		nread = read(channel->parent_to_child_fd[0], &tabRcv, sizeof(child_req_to_parent));
		if (nread != -1) {
			switch (tabRcv.type) {
				case CREATE_TAB:
					fprintf(stderr, "Tab %d Error: CREATE_TAB should not be received by normal tab\n", tab_index);
				break;
				case NEW_URI_ENTERED:
					fprintf(stderr, "Tab %d: Loading webpage\n", tab_index);
					render_web_page_in_tab(tabRcv.req.uri_req.uri, b_window);
				break;
				case TAB_KILLED:
					fprintf(stderr, "Tab %d: Received TAB_KILLED. Quit.\n", tab_index);
					process_all_gtk_events();
					free(channel);
					return 0;
				break;
			}
		}
	}
	free(channel);
	return 0;
}
/*
 * Name:				controller_process
 * Input arguments:	 'channel': Includes pipes to communctaion with
 *					  Router process
 * Output arguments:	none
 * Function:			This function will make a CONTROLLER window and
 *					  be blocked until the program terminates.
 */
int controller_process(comm_channel *channel) {
	// Do not need to change code in this function
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create controler window
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	show_browser();
	free(channel);
	fprintf(stderr, "Controller: Quit.\n");
	return 0;
}

void kill_all_child(int tab_pid_array[MAX_TAB],comm_channel *channel[MAX_TAB]) {
	fprintf(stderr, "Router: Sending request to kill all tabs\n");
	int i;
	child_req_to_parent tempReq;
	tempReq.type = TAB_KILLED;
	for (i = 1; i < MAX_TAB; i++) {
		if (tab_pid_array[i] != 0) {
			tempReq.req.killed_req.tab_index = i;
			write(channel[i]->parent_to_child_fd[1], &tempReq, sizeof(child_req_to_parent));
			waitpid(tab_pid_array[i], NULL, 0);
			free(channel[i]);
			tab_pid_array[i] = 0;
		}
	}
}

int init_pipe(comm_channel **channel) {
	int flags;
	*channel = (comm_channel *) malloc(sizeof(comm_channel));
	if (pipe((*channel)->child_to_parent_fd) == -1) {
		perror("pipe error");
		return -1;
	}
	flags = fcntl((*channel)->child_to_parent_fd[0], F_GETFL, 0);
	fcntl((*channel)->child_to_parent_fd[0], F_SETFL, flags | O_NONBLOCK);
	if (pipe((*channel)->parent_to_child_fd) == -1) {
		perror("pipe error");
		return -1;
	}
	return 0;
}

/*
 * Name:				router_process
 * Input arguments:	 none
 * Output arguments:	none
 * Function:			This function implements the logic of ROUTER process.
 *					  It will first create the CONTROLLER process  and then
 *					  polling request messages from all ite child processes.
 *					  It does not need to call any gtk library function.
 */

int router_process() {
	int parent_pid = getpid();
	comm_channel *channel[MAX_TAB];
	int i;
	int tab_pid_array[MAX_TAB] = {0}; // You can use this array to save the pid
									  // of every child process that has been
					  // created. When a chile process receives
					  // a TAB_KILLED message, you can call waitpid()
					  // to check if the process is indeed completed.
					  // This prevents the child process to become
					  // Zombie process. Another usage of this array
					  // is for bookkeeping which tab index has been
					  // taken.
	// Append your code here
	memset(tab_pid_array, 0, sizeof(tab_pid_array));
	// Prepare communication pipes with the CONTROLLER process
	int nread;
	int init_res = init_pipe(&channel[0]);
	if (init_res == -1) {
		return -1;
	}
	// Fork the CONTROLLER process
	int pid = fork();
	if (pid == 0) { //child
		// this is guard-processor for the controller.
		int restart_flag = 1;
		while (restart_flag) {
			restart_flag = 0;
			int guard_pid = fork();
			if (guard_pid == 0) { // sub-child
				controller_process(channel[0]);
				exit(0);
			}
			else if (guard_pid < 0) {
				perror("fork error in guard-processor");
				return -1;
			}
			//here is the guard-processor
			int status;
			waitpid(guard_pid, &status, 0);
			if (status != 0) { //unexpected quit
				fprintf(stderr, "Guard for Controller: Controller unexpected quit. Restarting.\n");
				restart_flag = 1;
			}
		}
		free(channel[0]);
		exit(0);
	}
	else if (pid < 0) { //parent
		perror("fork error");
		return -1;
	}
	// Here is parent.
	tab_pid_array[0] = pid;
	close(channel[0]->child_to_parent_fd[1]);
	close(channel[0]->parent_to_child_fd[0]);
	int id = 0;
	child_req_to_parent req_from_child;
	while (1) {
		usleep(1000);
		for (id = 0; id < MAX_TAB; id++) {
			if (tab_pid_array[id] == 0) {
				continue;
			}
			nread = read(channel[id]->child_to_parent_fd[0], &req_from_child, sizeof(child_req_to_parent));
			if (nread == -1) {
				// no message income
			}
			else {
				// have message income
				int tab_num;
				switch (req_from_child.type) {
					case CREATE_TAB:
						if (id != 0) {
							fprintf(stderr, "Router Error: CREATE_TAB must be sent by controller.\n");
						}
						else {
							tab_num = -1;
							for (i = 1; tab_num == -1 && i < MAX_TAB; i++) {
								if (tab_pid_array[i] == 0) {
									tab_num = i;
								}
							}
							if (tab_num == -1) {
								fprintf(stderr, "Router Error: The number of tabs reaches the max limit.\n");
								continue;
							}
							fprintf(stderr, "Router: Create tab %d\n", tab_num);

							init_res = init_pipe(&channel[tab_num]);
							if (init_res == -1) {
								return -1;
							}

							pid = fork();
							if (pid == 0) { //child
								url_rendering_process(tab_num, channel[tab_num], parent_pid);
								exit(0);
							}
							else if (pid > 0) { //parent
								tab_pid_array[tab_num] = pid;
								close(channel[tab_num]->child_to_parent_fd[1]);
								close(channel[tab_num]->parent_to_child_fd[0]);
							}
							else {
								perror("fork error");
								return -1;
							}
						}
					break;

					case NEW_URI_ENTERED:
						if (id != 0) {
							fprintf(stderr, "Router Error: NEW_URI_ENTERED must be sent by controller.\n");
						}
						else {
							int tab_id = req_from_child.req.uri_req.render_in_tab;
							if (tab_pid_array[tab_id] != 0) {
								write(channel[tab_id]->parent_to_child_fd[1], &req_from_child, sizeof(child_req_to_parent));
							}
							else {
								fprintf(stderr, "Router Error: url entered for non-existent Tab (%d)\n", tab_id);
							}
						}
					break;

					case TAB_KILLED:
						fprintf(stderr, "Router: Killing msg received from tab %d\n", id);
						if (req_from_child.req.killed_req.tab_index != 0) {
							int temp_id = req_from_child.req.killed_req.tab_index;
							if (tab_pid_array[temp_id] != 0) {
								write(channel[temp_id]->parent_to_child_fd[1],
									  &req_from_child,
									  sizeof(child_req_to_parent)); //send back the packet to kill the processor.
								waitpid(tab_pid_array[temp_id], NULL, 0);
								free(channel[temp_id]);
								tab_pid_array[temp_id] = 0;
							}
						}
						else {
							waitpid(tab_pid_array[0], NULL, 0);
							free(channel[0]);
							tab_pid_array[0] = 0;
							kill_all_child(tab_pid_array, channel);
							fprintf(stderr, "Router: Exit successfully.\n");
							return 0;
						}
					break;
				}
			}
		}
	}
	return 0;
}

int main() {
	return router_process();
}
