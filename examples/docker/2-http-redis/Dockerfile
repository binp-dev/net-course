FROM python:latest

WORKDIR /srv

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY server.py .

CMD flask --app server run -h 0.0.0.0 -p 80
