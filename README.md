~~~
＊ CSci4061 F2016 Assignment 2
＊ login: zhou0745
＊ date: 10/28/2016
＊ name: Tiannan Zhou, Annelies Odermann, Lidiya Dergacheva
＊ id: 5232494 (zhou0745), 4740784 (oderm008), 4515808 (derg0004)
~~~

**How to compile the program.**

~~~
make
~~~

You just need to use GNU makefile to compile this project.

**Who did what on the program**
* Tiannan Zhou
	* Completed CREATE_TAB, TAB_KILLED part and some error-control code.
* Annelies Odermann
	* Completed NEW_URI_ENTERED part.
* Lidiya Dergacheva
	* TBD.

**Usage**
* Start
	* Run `./browser` in the terminal to start the browser.
* Basic Operation
	* Click the '+' button on the Controller to create a new tab.
    * Enter URL in the URL-Region and tab number in the Tab-selector Region. We will check whether the tab is existent then load the website on the assigned tab if the tab number entered is valid.
    * Close the Controller Windows to quit the program.
* Error Handle
    * We will kill all tab windows and tab processes then quit the program if the Controller meets unexpected exit.
    * We will kill all tab windows and tab processes if the Router meets unexpected exit. You still need to close the Controller window manually for halting the Controller process.
