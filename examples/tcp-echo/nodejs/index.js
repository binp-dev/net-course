import { createServer } from 'net';

const server = createServer((socket) => {
    const addr = `${socket.remoteAddress}:${socket.remotePort}`

    console.log(`Accepted: ${addr}`);

    socket.on('end', () => {
        console.log(`Closed: ${addr}`);
    });

    socket.pipe(socket);
});

server.on('error', (err) => {
    throw err;
});

server.listen(8080, '127.0.0.1', () => {
    const addr = server.address()
    console.log(`Listen as ${JSON.stringify(addr)}`);
});
