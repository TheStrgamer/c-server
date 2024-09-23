This is a simple http server written in c

It can parse html files with commands
Commands are put in the html and are written like this:
{ Function arg }
Where function is the function to use, and arg is for example a txt file.

So far, the commands that have functionality
* text
* list

text pust the txt file content in the html as pure text
It could be put in for example a ```<p>```

list parses the content of the txt file as <li> elements

Here are some examples of implementations

<p>
{ text example.txt }
</p>

<Table>
{ list names.txt }
</Table>