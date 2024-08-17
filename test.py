def loop_test():
    a = 0
    b = 0
    for i in range(50):
        a += i
        for j in range(10):
            b = a + j
    return b

# print(loop_test())

def fib(x):
    if x < 3:
        return 1
    return fib(x-1) + fib(x-2)

print(fib(10))
