rm *.pem *.srl *.crt
# 1. Generate CA's private key and self-sign it 
openssl req -x509 -newkey rsa:4096 -days 3650 -subj "/C=AU/ST=NSW/L=Sydney/O=Avo-Kouyoumijian/CN=AVOCHE-CA/emailAddress=avokouyoumjian1@gmail.com" -keyout avoche-ca-key.pem -out avoche-ca-crt.pem

# 2. Generate a web server's private key and certificate signing request
openssl req -newkey rsa:4096 -subj "/C=AU/ST=NSW/L=Sydney/O=Avo-Kouyoumijian/CN=AVOCHE-webserver/emailAddress=avokouyoumjian1@gmail.com" -keyout server-key.pem -out server-req.pem  -nodes

# 3. Use CA's private key to sign the web server certificate
openssl x509 -req -in server-req.pem -CA avoche-ca-crt.pem -CAkey avoche-ca-key.pem -CAcreateserial -out server-crt.pem -days 3650 -extfile server-ext.cnf


sudo rm fullchain.pem
cat server-crt.pem > fullchain.pem
cat avoche-ca-crt.pem >> fullchain.pem

sudo mkdir -p /usr/local/share/ca-certificates/
sudo cp avoche-ca-crt.pem /usr/local/share/ca-certificates/avoche-ca-crt.crt
sudo update-ca-certificates
