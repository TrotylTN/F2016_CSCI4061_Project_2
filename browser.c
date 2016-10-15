#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TAB 100
#define MSGSIZE 1024
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
	if(tab_index <= 0) {
		fprintf(stderr, "Invalid tab index (%d).", tab_index);
		return;
	}
	// Get the URL.
	char * uri = get_entered_uri(entry);
	// Append your code here
	// ------------------------------------
	// * Prepare a NEW_URI_ENTERED packet to send to ROUTER (parent) process.
	req_type tab_msg = NEW_URI_ENTERED;
	close(channel.child_to_parent_fd[0]);
	write(channel.child_to_parent_fd[1], &tab_msg, sizeof(req_type));
	// * Send the url and tab index to ROUTER
	write(channel.child_to_parent_fd[1], uri, MSGSIZE);
	write(channel.child_to_parent_fd[1], &tab_index, sizeof(int));
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
	// ------------------------------------
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
int url_rendering_process(int tab_index, comm_channel *channel) {
	// Don't forget to close pipe fds which are unused by this process
	browser_window * b_window = NULL;
	// Create url-rendering window
	create_browser(URL_RENDERING_TAB, tab_index, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	child_req_to_parent req;
	while (1) {
		// Handle one gtk event, you don't need to change it nor understand what it does.
		process_single_gtk_event();
		// Poll message from ROUTER
		// It is unnecessary to poll requests unstoppably, that will consume too much CPU time
		// Sleep some time, e.g. 1 ms, and render CPU to other processes
		usleep(1000);
		// Append your code here
		// Try to read data sent from ROUTER
		// If no data being read, go back to the while loop
		// Otherwise, check message type:
		//   * NEW_URI_ENTERED
		//	 ** call render_web_page_in_tab(req.req.uri_req.uri, b_window);
		//   * TAB_KILLED
		//	 ** call process_all_gtk_events() to process all gtk events and jump out of the loop
		//   * CREATE_TAB or unknown message type
		//	 ** print an error message and ignore it
		// Handle read error, e.g. what if the ROUTER has quit unexpected?
	}
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
	comm_channel *channel[MAX_TAB];
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
	// Prepare communication pipes with the CONTROLLER process
	channel[0] = (comm_channel *) malloc(sizeof(comm_channel));
	if (pipe(channel[0]->child_to_parent_fd) == -1) {
		perror("pipe error");
		return -1;
	}
	int flags, nread;
	flags = fcntl(channel[0]->child_to_parent_fd[0], F_GETFL, 0);
	fcntl(channel[0]->child_to_parent_fd[0], F_SETFL, flags | O_NONBLOCK);

	if (pipe(channel[0]->parent_to_child_fd) == -1) {
		perror("pipe error");
		return -1;
	}
	// Fork the CONTROLLER process
	int pid = fork();
	if (pid == 0) { //child
		controller_process(channel[0]);
		exit(0);
	}
	else if (pid > 0) { //parent
		tab_pid_array[0] = pid;
	}
	//   call controller_process() in the forked CONTROLLER process
	// Don't forget to close pipe fds which are unused by this process
	// Poll child processes' communication channels using non-blocking pipes.
	// Before any other URL-RENDERING process is created, CONTROLLER process
	// is the only child process. When one or more URL-RENDERING processes
	// are created, you would also need to poll their communication pipe.
	while (true) {
		req_type tab_msg;
		close(channel[0]->child_to_parent_fd[1]);
		nread = read(channel[0]->child_to_parent_fd[0], &tab_msg, sizeof(req_type));
		if (nread == -1) {
			fprintf(stderr, "No message income\n");
			usleep(100);
		}
		else {
			switch (tab_msg) {
				case CREATE_TAB:

				break;
				case NEW_URI_ENTERED:
					char * uri;
					int tab_index;
					read(channel[0]->child_to_parent_fd[0], &uri, MSGSIZE);
					read(channel[0]->child_to_parent_fd[0], &tab_index, sizeof(int));

				break;
				case TAB_KILLED:
				break;
			}
		}
	}
	//   * sleep some time if no message received
	//   * if message received, handle it:
	//	 ** CREATE_TAB:
	//
	//		Prepare communication pipes with the new URL-RENDERING process
	//		Fork the new URL-RENDERING process
	//
	//	 ** NEW_URI_ENTERED:
	//
	//		Send TAB_KILLED message to the URL-RENDERING process in which
	//		the new url is going to be rendered
	//
	//	 ** TAB_KILLED:
	//
	//		If the killed process is the CONTROLLER process
	//		*** send TAB_KILLED messages to kill all the URL-RENDERING processes
	//		*** call waitpid on every child URL-RENDERING processes
	//		*** self exit
	//
	//		If the killed process is a URL-RENDERING process
	//		*** send TAB_KILLED to the URL-RENDERING
	//		*** call waitpid on every child URL-RENDERING processes
	//		*** close pipes for that specific process
	//		*** remove its pid from tab_pid_array[]
	//
	return 0;
}

int main() {
	return router_process();
}
