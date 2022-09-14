use async_std::{
    io,
    net::{TcpListener, TcpStream},
    prelude::*,
    task,
};

async fn process(stream: TcpStream) -> io::Result<()> {
    let peer_addr = stream.peer_addr()?;
    println!("Accepted: {}", peer_addr);

    let mut reader = stream.clone();
    let mut writer = stream;
    io::copy(&mut reader, &mut writer).await?;

    println!("Closed: {}", peer_addr);
    Ok(())
}

fn main() -> io::Result<()> {
    task::block_on(async {
        let listener = TcpListener::bind("127.0.0.1:8080").await?;
        println!("Listening on {}", listener.local_addr()?);

        let mut incoming = listener.incoming();

        while let Some(stream) = incoming.next().await {
            let stream = stream?;
            task::spawn(async {
                process(stream).await.unwrap();
            });
        }
        Ok(())
    })
}
