from main import Arguments
from fastapi import FastAPI

from util import process_data

from inference import load_model


app = FastAPI()

model = load_model()

@app.get("/")
async def root():
    process_data(model)
    return {"message": "Request Received"}
    

# if __name__ == "__main__":
#     model = load_model()


