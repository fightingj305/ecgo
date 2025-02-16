import torch
import numpy as np  
from models import build_model
import torch.nn.functional as F
from main import Arguments


def load_model():

    # class Arguments(object):
    #     def __init__(self) -> None:
    #         print(f"This machine has {torch.cuda.device_count()} gpu...")
    #         self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    #         self.rootpath = r"C:\Users\kelvi\03 MyDocuments\30 MyCode\TreeHacks 2025\ECG-arrhythmia-detection-based-on-DETR\\" ## dataset path
    #         self.numfolds = 10
    #         self.seed = 10086
    #         self.batchsize = 128
    #         self.epochs = 150
    #         self.clip_max_norm = 0.15
    #         self.lr_drop = 80
    #         self.output_dir = "./outputs/"
    #         self.early_stop_patience = 10  # Number of epochs with no improvement after which training will be stopped
    model_path = r"C:\Users\kelvi\03 MyDocuments\30 MyCode\TreeHacks 2025\ECG-arrhythmia-detection-based-on-DETR\outputs\best_checkpoint.pth"
    model_file = torch.load(model_path, weights_only=False)
    print("Loading model weights from:", model_path)
    model_weights = model_file['model']

    in_chan, d_model, num_class, num_queries, aux_loss = 1, 128, 5, 10, True
    model, _, _ = build_model(in_chan, d_model, num_class, num_queries, aux_loss=aux_loss)

    model.load_state_dict(model_weights, strict=False)
    model.to("cuda")
    model.eval()
    print("Model loaded successfully.")

    return model
def forward(model, data):
    # Load the model weights
    # model_path = r"C:\Users\kelvi\03 MyDocuments\30 MyCode\TreeHacks 2025\ECG-arrhythmia-detection-based-on-DETR\outputs\best_checkpoint.pth"
    # model_file = torch.load(model_path, map_location=torch.device('cpu'))
    # print("Loading model weights from:", model_path)
    # model_weights = model_file['model']

    # Load the model architecture and weights

    target_sizes = torch.tensor([1080])
    
    outputs = model(torch.tensor(data, dtype=torch.float32).unsqueeze(0).unsqueeze(0).to("cuda"))


    out_logits, out_box = outputs["pred_logits"], outputs["pred_boxes"]
    # pdb.set_trace()
    assert len(out_logits) == len(target_sizes)
    assert target_sizes.ndim == 1

    prob = F.softmax(out_logits, dim=-1)
    scores, labels = prob[..., :-1].max(dim=-1) 

    results = [{"scores": s, "labels": l} for s, l, in zip(scores, labels)][0]

    if results['scores'][0] > results['scores'][1]:
        return 0
    else:
        return 1





