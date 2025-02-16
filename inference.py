import torch
import numpy as np  
from models import build_model

class Arguments(object):
    def __init__(self) -> None:
        print(f"This machine has {torch.cuda.device_count()} gpu...")
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        self.rootpath = r"C:\Users\kelvi\03 MyDocuments\30 MyCode\TreeHacks 2025\ECG-arrhythmia-detection-based-on-DETR\\" ## dataset path
        self.numfolds = 10
        self.seed = 10086
        self.batchsize = 128
        self.epochs = 150
        self.clip_max_norm = 0.15
        self.lr_drop = 80
        self.output_dir = "./outputs/"
        self.early_stop_patience = 10  # Number of epochs with no improvement after which training will be stopped
def get_data(data):
    pass

model_path = r"C:\Users\kelvi\03 MyDocuments\30 MyCode\TreeHacks 2025\ECG-arrhythmia-detection-based-on-DETR\outputs\best_checkpoint.pth"
model_file = torch.load(model_path weights_only=False)
print("Loading model weights from:", model_path)
model_weights = model_file['model']

in_chan, d_model, num_class, num_queries, aux_loss = 1, 128, 5, 10, True
model, _, _ = build_model(in_chan, d_model, num_class, num_queries, aux_loss=aux_loss)

model.load_state_dict(model_weights, strict=False)
model.to(Arguments().device)
model.eval()
print("Model loaded successfully.")

data = get_data("test")




