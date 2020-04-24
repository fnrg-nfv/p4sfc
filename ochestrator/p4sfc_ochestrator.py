from flask import Flask, request
app = Flask(__name__)


@app.route('/test')
def test():
    return "Hello from p4sfc ochestrator\n"


if __name__ == '__main__':
    app.run(host="0.0.0.0", port='8091')
