import httpparser

h=httpparser.Http()
print(h.feed("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 1))
print(h.host())

print("----")

h=httpparser.Http()
print(h.feed("GET / HTTP/1.1\r\n", 0))
print(h.feed("Host: localhost\r\n", 0))
#print(h.feed("Foo: localhost\r\n", 0))
print(h.feed("\r\n", 1))
print(h.host())
