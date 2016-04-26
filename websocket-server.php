<?php
  use Ratchet\MessageComponentInterface;
  use Ratchet\ConnectionInterface;
  use Ratchet\Server\IoServer;
  use Ratchet\WebSocket\WsServer;
  use Ratchet\Http\HttpServer;

  require __DIR__.'/vendor/autoload.php';
  
  //example: 7F_man empty 439

  /**
  * chat.php
  * Send any incoming messages to all connected clients (except sender)
  */
  class Chat implements MessageComponentInterface {

      protected $clients;
      protected $statusArray = [];

      public function __construct() {
          $this->clients = new \SplObjectStorage;
      }

      public function onOpen(ConnectionInterface $conn) {
          $this->clients->attach($conn);
      }

      public function onMessage(ConnectionInterface $from, $msg) {
          foreach ($this->clients as $client) {
              if ($from != $client) {
                  $client->send($msg);
              }
          }
          $msgArray = explode(" ", $msg);
          if (count($msgArray) >= 3) {
             if($msgArray[1] == 'empty' || $msgArray[1] == 'full') {
              if (! isset($this->statusArray[$msgArray[0]]) || $this->statusArray[$msgArray[0]]['state'] != $msgArray[1]) {
                  echo "[changed]:{$msgArray[0]} {$msgArray[1]} {$msgArray[2]}\n";
                  $diff = (isset($this->statusArray[$msgArray[0]]['time'])) ? time() - $this->statusArray[$msgArray[0]]['time'] : 0;
                  $log = json_encode(['name' => $msgArray[0], 'status' => ($msgArray[1] == 'empty') ? 'full' : 'empty', 'diff' => $diff]);
                  file_put_contents("/var/log/toilet.log", "{$log}\n", FILE_APPEND);
                  $this->statusArray[$msgArray[0]] = ['state' => $msgArray[1], 'time' => time()];
                }
              }
          }
          echo "[msg]:" . $msg . "\n";
      }

      public function onClose(ConnectionInterface $conn) {
          $this->clients->detach($conn);
      }

      public function onError(ConnectionInterface $conn, \Exception $e) {
          $conn->close();
      }
  }

  // Run the server application through the WebSocket protocol on port 8080
  $server = IoServer::factory(new HttpServer(new WsServer(new Chat())), 8484);
  $server->run();
