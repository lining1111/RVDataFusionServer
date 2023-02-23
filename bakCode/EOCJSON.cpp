//
// Created by lining on 2023/2/23.
//

#include "EOCJSON.h"

bool ReqHead::JsonMarshal(Json::Value &out) {
    out["Guid"] = this->Guid;
    out["Version"] = this->Version;
    out["Code"] = this->Code;

    return true;
}

bool ReqHead::JsonUnmarshal(Json::Value in) {
    this->Guid = in["Guid"].asString();
    this->Version = in["Version"].asString();
    this->Code = in["Code"].asString();

    return true;
}

bool RspHead::JsonMarshal(Json::Value &out) {
    out["Guid"] = this->Guid;
    out["Version"] = this->Version;
    out["Code"] = this->Code;
    out["State"] = this->State;
    out["ResultMessage"] = this->ResultMessage;
    return true;
}

bool RspHead::JsonUnmarshal(Json::Value in) {
    this->Guid = in["Guid"].asString();
    this->Version = in["Version"].asString();
    this->Code = in["Code"].asString();
    this->State = in["State"].asInt();
    this->ResultMessage = in["ResultMessage"].asString();

    return true;
}

bool DataS100::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    return true;
}

bool DataS100::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    return true;
}

bool S100::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S100::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR100::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    return true;
}

bool DataR100::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    return true;
}

bool R100::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R100::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS101::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["EquipNumber"] = this->EquipNumber;
    out["EquipIp"] = this->EquipIp;
    out["EquipType"] = this->EquipType;
    out["SoftVersion"] = this->SoftVersion;
    out["DataVersion"] = this->DataVersion;
    return true;
}

bool DataS101::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->EquipNumber = in["EquipNumber"].asString();
    this->EquipIp = in["EquipIp"].asString();
    this->EquipType = in["EquipType"].asString();
    this->SoftVersion = in["SoftVersion"].asString();
    this->DataVersion = in["DataVersion"].asString();
    return true;
}

bool S101::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S101::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }
    return true;
}

bool DataR101::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["State"] = this->State;
    out["Message"] = this->Message;
    return true;
}

bool DataR101::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->State = in["State"].asInt();
    this->Message = in["Message"].asString();
    return true;
}

bool R101::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R101::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }
    return true;
}

bool IntersectionBaseSettingEntity::JsonMarshal(Json::Value &out) {
    out["FlagEast"] = this->FlagEast;
    out["FlagSouth"] = this->FlagSouth;
    out["FlagWest"] = this->FlagWest;
    out["FlagNorth"] = this->FlagNorth;
    out["DeltaXEast"] = this->DeltaXEast;
    out["DeltaYEast"] = this->DeltaYEast;
    out["DeltaXSouth"] = this->DeltaXSouth;
    out["DeltaYSouth"] = this->DeltaYSouth;
    out["DeltaXWest"] = this->DeltaXWest;
    out["DeltaYWest"] = this->DeltaYWest;
    out["DeltaXNorth"] = this->DeltaXNorth;
    out["DeltaYNorth"] = this->DeltaYNorth;
    out["WidthX"] = this->WidthX;
    out["WidthY"] = this->WidthY;

    return true;
}

bool IntersectionBaseSettingEntity::JsonUnmarshal(Json::Value in) {
    this->FlagEast = in["FlagEast"].asInt();
    this->FlagSouth = in["FlagSouth"].asInt();
    this->FlagWest = in["FlagWest"].asInt();
    this->FlagNorth = in["FlagNorth"].asInt();
    this->DeltaXEast = in["DeltaXEast"].asFloat();
    this->DeltaYEast = in["DeltaYEast"].asFloat();
    this->DeltaXSouth = in["DeltaXSouth"].asFloat();
    this->DeltaYSouth = in["DeltaYSouth"].asFloat();
    this->DeltaXWest = in["DeltaXWest"].asFloat();
    this->DeltaYWest = in["DeltaYWest"].asFloat();
    this->DeltaXNorth = in["DeltaXNorth"].asFloat();
    this->DeltaYNorth = in["DeltaYNorth"].asFloat();
    this->WidthX = in["WidthX"].asFloat();
    this->WidthY = in["WidthY"].asFloat();
    return true;
}

bool IntersectionEntity::JsonMarshal(Json::Value &out) {
    out["Guid"] = this->Guid;
    out["Name"] = this->Name;
    out["Type"] = this->Type;
    out["PlatId"] = this->PlatId;
    out["XLength"] = this->XLength;
    out["YLength"] = this->YLength;
    out["LaneNumber"] = this->LaneNumber;
    out["Latitude"] = this->Latitude;
    out["longitude"] = this->longitude;

    Json::Value IntersectionBaseSetting = Json::objectValue;
    this->IntersectionBaseSetting.JsonMarshal(IntersectionBaseSetting);
    out["IntersectionBaseSetting"] = IntersectionBaseSetting;
    return true;
}

bool IntersectionEntity::JsonUnmarshal(Json::Value in) {
    this->Guid = in["Guid"].asString();
    this->Name = in["Name"].asString();
    this->Type = in["Type"].asInt();
    this->PlatId = in["PlatId"].asString();
    this->XLength = in["XLength"].asFloat();
    this->YLength = in["YLength"].asFloat();
    this->LaneNumber = in["LaneNumber"].asInt();
    this->Latitude = in["Latitude"].asString();
    this->longitude = in["longitude"].asString();

    Json::Value IntersectionBaseSetting = in["IntersectionBaseSetting"];
    if (IntersectionBaseSetting.isObject()) {
        this->IntersectionBaseSetting.JsonUnmarshal(IntersectionBaseSetting);
    } else {
        return false;
    }

    return true;
}

bool BaseSettingEntity::JsonMarshal(Json::Value &out) {
    out["City"] = this->City;
    out["IsUploadToPlatform"] = this->IsUploadToPlatform;
    out["Is4Gmodel"] = this->Is4Gmodel;
    out["IsIllegalCapture"] = this->IsIllegalCapture;
    out["IsPrintIntersectionName"] = this->IsPrintIntersectionName;
    out["Remarks"] = this->Remarks;
    out["FilesServicePath"] = this->FilesServicePath;
    out["FilesServicePort"] = this->FilesServicePort;
    out["MainDNS"] = this->MainDNS;
    out["AlternateDNS"] = this->AlternateDNS;
    out["PlatformTcpPath"] = this->PlatformTcpPath;
    out["PlatformTcpPort"] = this->PlatformTcpPort;
    out["PlatformHttpPath"] = this->PlatformHttpPath;
    out["PlatformHttpPort"] = this->PlatformHttpPort;
    out["SignalMachinePath"] = this->SignalMachinePath;
    out["SignalMachinePort"] = this->SignalMachinePort;
    out["IsUseSignalMachine"] = this->IsUseSignalMachine;
    out["NtpServerPath"] = this->NtpServerPath;
    out["IllegalPlatformAddress"] = this->IllegalPlatformAddress;
    out["FusionMainboardIp"] = this->FusionMainboardIp;
    out["FusionMainboardPort"] = this->FusionMainboardPort;

    return true;
}

bool BaseSettingEntity::JsonUnmarshal(Json::Value in) {
    this->City = in["City"].asString();
    this->IsUploadToPlatform = in["IsUploadToPlatform"].asInt();
    this->Is4Gmodel = in["Is4Gmodel"].asInt();
    this->IsIllegalCapture = in["IsIllegalCapture"].asInt();
    this->IsPrintIntersectionName = in["IsPrintIntersectionName"].asInt();
    this->Remarks = in["Remarks"].asString();
    this->FilesServicePath = in["FilesServicePath"].asString();
    this->FilesServicePort = in["FilesServicePort"].asInt();
    this->MainDNS = in["MainDNS"].asString();
    this->AlternateDNS = in["AlternateDNS"].asString();
    this->PlatformTcpPath = in["PlatformTcpPath"].asString();
    this->PlatformTcpPort = in["PlatformTcpPort"].asInt();
    this->PlatformHttpPath = in["PlatformHttpPath"].asString();
    this->PlatformHttpPort = in["PlatformHttpPort"].asInt();
    this->SignalMachinePath = in["SignalMachinePath"].asString();
    this->SignalMachinePort = in["SignalMachinePort"].asInt();
    this->IsUseSignalMachine = in["IsUseSignalMachine"].asInt();
    this->NtpServerPath = in["NtpServerPath"].asString();
    this->IllegalPlatformAddress = in["IllegalPlatformAddress"].asString();
    this->FusionMainboardIp = in["FusionMainboardIp"].asString();
    this->FusionMainboardPort = in["FusionMainboardPort"].asInt();

    return true;
}

bool FusionParaEntity::JsonMarshal(Json::Value &out) {
    out["IntersectionAreaPoint1X"] = this->IntersectionAreaPoint1X;
    out["IntersectionAreaPoint1Y"] = this->IntersectionAreaPoint1Y;
    out["IntersectionAreaPoint2X"] = this->IntersectionAreaPoint2X;
    out["IntersectionAreaPoint2Y"] = this->IntersectionAreaPoint2Y;
    out["IntersectionAreaPoint3X"] = this->IntersectionAreaPoint3X;
    out["IntersectionAreaPoint3Y"] = this->IntersectionAreaPoint3Y;
    out["IntersectionAreaPoint4X"] = this->IntersectionAreaPoint4X;
    out["IntersectionAreaPoint4Y"] = this->IntersectionAreaPoint4Y;
    return true;
}

bool FusionParaEntity::JsonUnmarshal(Json::Value in) {
    this->IntersectionAreaPoint1X = in["IntersectionAreaPoint1X"].asFloat();
    this->IntersectionAreaPoint1Y = in["IntersectionAreaPoint1Y"].asFloat();
    this->IntersectionAreaPoint2X = in["IntersectionAreaPoint2X"].asFloat();
    this->IntersectionAreaPoint2Y = in["IntersectionAreaPoint2Y"].asFloat();
    this->IntersectionAreaPoint3X = in["IntersectionAreaPoint3X"].asFloat();
    this->IntersectionAreaPoint3Y = in["IntersectionAreaPoint3Y"].asFloat();
    this->IntersectionAreaPoint4X = in["IntersectionAreaPoint4X"].asFloat();
    this->IntersectionAreaPoint4Y = in["IntersectionAreaPoint4Y"].asFloat();

    return true;
}

bool AssociatedEquip::JsonMarshal(Json::Value &out) {
    out["EquipType"] = this->EquipType;
    out["EquipCode"] = this->EquipCode;

    return true;
}

bool AssociatedEquip::JsonUnmarshal(Json::Value in) {
    this->EquipType = in["EquipType"].asInt();
    this->EquipCode = in["EquipCode"].asString();

    return true;
}

bool DataR102::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["DataVersion"] = this->DataVersion;
    out["Index"] = this->Index;

    Json::Value IntersectionInfo = Json::objectValue;
    this->IntersectionInfo.JsonMarshal(IntersectionInfo);
    out["IntersectionInfo"] = IntersectionInfo;

    Json::Value BaseSetting = Json::objectValue;
    this->BaseSetting.JsonMarshal(BaseSetting);
    out["BaseSetting"] = BaseSetting;

    Json::Value FusionParaSetting = Json::objectValue;
    this->FusionParaSetting.JsonMarshal(FusionParaSetting);
    out["FusionParaSetting"] = FusionParaSetting;

    Json::Value AssociatedEquips = Json::arrayValue;
    if (!this->AssociatedEquips.empty()) {
        for (auto iter: this->AssociatedEquips) {
            Json::Value item;
            iter.JsonMarshal(item);
            AssociatedEquips.append(item);
        }
    } else {
        AssociatedEquips.resize(0);
    }

    out["AssociatedEquips"] = AssociatedEquips;

    return true;
}

bool DataR102::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->DataVersion = in["DataVersion"].asString();
    this->Index = in["Index"].asInt();

    Json::Value IntersectionInfo = in["IntersectionInfo"];
    if (IntersectionInfo.isObject()) {
        this->IntersectionInfo.JsonUnmarshal(IntersectionInfo);
    }

    Json::Value BaseSetting = in["BaseSetting"];
    if (BaseSetting.isObject()) {
        this->BaseSetting.JsonUnmarshal(BaseSetting);
    }

    Json::Value FusionParaSetting = in["FusionParaSetting"];
    if (FusionParaSetting.isObject()) {
        this->FusionParaSetting.JsonUnmarshal(FusionParaSetting);
    }

    Json::Value AssociatedEquips = in["AssociatedEquips"];
    if (AssociatedEquips.isArray()) {
        for (auto iter:AssociatedEquips) {
            AssociatedEquip item;
            if (item.JsonUnmarshal(iter)) {
                this->AssociatedEquips.push_back(item);
            }
        }
    }

    return true;
}

bool R102::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R102::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS102::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["State"] = this->State;
    out["Messsage"] = this->Messsage;
    return true;
}

bool DataS102::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->State = in["State"].asInt();
    this->Messsage = in["Messsage"].asString();

    return true;
}

bool S102::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S102::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR104::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["DataVersion"] = this->DataVersion;
    out["Index"] = this->Index;

    Json::Value IntersectionInfo = Json::objectValue;
    this->IntersectionInfo.JsonMarshal(IntersectionInfo);
    out["IntersectionInfo"] = IntersectionInfo;

    Json::Value BaseSetting = Json::objectValue;
    this->BaseSetting.JsonMarshal(BaseSetting);
    out["BaseSetting"] = BaseSetting;

    Json::Value FusionParaSetting = Json::objectValue;
    this->FusionParaSetting.JsonMarshal(FusionParaSetting);
    out["FusionParaSetting"] = FusionParaSetting;

    Json::Value AssociatedEquips = Json::arrayValue;
    if (!this->AssociatedEquips.empty()) {
        for (auto iter: this->AssociatedEquips) {
            Json::Value item;
            iter.JsonMarshal(item);
            AssociatedEquips.append(item);
        }
    } else {
        AssociatedEquips.resize(0);
    }

    out["AssociatedEquips"] = AssociatedEquips;

    return true;
}

bool DataR104::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->DataVersion = in["DataVersion"].asString();
    this->Index = in["Index"].asInt();

    Json::Value IntersectionInfo = in["IntersectionInfo"];
    if (IntersectionInfo.isObject()) {
        this->IntersectionInfo.JsonUnmarshal(IntersectionInfo);
    }

    Json::Value BaseSetting = in["BaseSetting"];
    if (BaseSetting.isObject()) {
        this->BaseSetting.JsonUnmarshal(BaseSetting);
    }

    Json::Value FusionParaSetting = in["FusionParaSetting"];
    if (FusionParaSetting.isObject()) {
        this->FusionParaSetting.JsonUnmarshal(FusionParaSetting);
    }

    Json::Value AssociatedEquips = in["AssociatedEquips"];
    if (AssociatedEquips.isArray()) {
        for (auto iter:AssociatedEquips) {
            AssociatedEquip item;
            if (item.JsonUnmarshal(iter)) {
                this->AssociatedEquips.push_back(item);
            }
        }
    }

    return true;
}

bool R104::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R104::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS104::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["MainboardGuid"] = this->MainboardGuid;

    return true;
}

bool DataS104::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->MainboardGuid = in["MainboardGuid"].asString();

    return true;
}

bool S104::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S104::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS105::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["Total"] = this->Total;
    out["Success"] = this->Success;

    return true;
}

bool DataS105::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->Total = in["Total"].asInt();
    this->Success = in["Success"].asInt();

    return true;
}

bool S105::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S105::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR105::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["State"] = this->State;
    out["Message"] = this->Message;
    return true;
}

bool DataR105::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->State = in["State"].asInt();
    this->Message = in["Message"].asString();
    return true;
}

bool R105::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R105::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR106::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["FileName"] = this->FileName;
    out["FileSize"] = this->FileSize;
    out["FileVersion"] = this->FileVersion;
    out["DownloadPath"] = this->DownloadPath;
    out["FileMD5"] = this->FileMD5;

    return true;
}

bool DataR106::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->FileName = in["FileName"].asString();
    this->FileSize = in["FileSize"].asString();
    this->FileVersion = in["FileVersion"].asString();
    this->DownloadPath = in["DownloadPath"].asString();
    this->FileMD5 = in["FileMD5"].asString();

    return true;
}

bool R106::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R106::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS106::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["ResultType"] = this->ResultType;
    out["State"] = this->State;
    out["Progress"] = this->Progress;
    out["Message"] = this->Message;

    return true;
}

bool DataS106::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->ResultType = in["ResultType"].asInt();
    this->State = in["State"].asInt();
    this->Progress = in["Progress"].asInt();
    this->Message = in["Message"].asString();

    return true;
}

bool S106::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S106::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR107::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["FileName"] = this->FileName;
    out["FileVersion"] = this->FileVersion;
    out["UpgradeMode"] = this->UpgradeMode;
    out["FileMD5"] = this->FileMD5;
    out["CurrentSoftVersion"] = this->CurrentSoftVersion;
    out["HardwareVersion"] = this->HardwareVersion;

    return true;
}

bool DataR107::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->FileName = in["FileName"].asString();
    this->FileVersion = in["FileVersion"].asString();
    this->UpgradeMode = in["UpgradeMode"].asInt();
    this->FileMD5 = in["FileMD5"].asString();
    this->CurrentSoftVersion = in["CurrentSoftVersion"].asString();
    this->HardwareVersion = in["HardwareVersion"].asString();

    return true;
}

bool R107::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R107::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS107::JsonMarshal(Json::Value &out) {
    out["Code"] = this->Code;
    out["ResultType"] = this->ResultType;
    out["State"] = this->State;
    out["Progress"] = this->Progress;
    out["Message"] = this->Message;

    return true;
}

bool DataS107::JsonUnmarshal(Json::Value in) {
    this->Code = in["Code"].asString();
    this->ResultType = in["ResultType"].asInt();
    this->State = in["State"].asInt();
    this->Progress = in["Progress"].asInt();
    this->Message = in["Message"].asString();

    return true;
}

bool S107::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S107::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataR108::JsonMarshal(Json::Value &out) {
    out["Code"]=this->Code;
    return true;
}

bool DataR108::JsonUnmarshal(Json::Value in) {
    this->Code=in["Code"].asString();
    return true;
}

bool R108::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool R108::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}

bool DataS108::JsonMarshal(Json::Value &out) {
    out["Code"]=this->Code;
    return true;
}

bool DataS108::JsonUnmarshal(Json::Value in) {
    this->Code=in["Code"].asString();
    return true;
}

bool S108::JsonMarshal(Json::Value &out) {
    this->head.JsonMarshal(out);
    Json::Value Data = Json::objectValue;
    this->Data.JsonMarshal(Data);
    out["Data"] = Data;
    return true;
}

bool S108::JsonUnmarshal(Json::Value in) {
    this->JsonUnmarshal(in);
    Json::Value Data = in["Data"];
    if (Data.isObject()) {
        this->Data.JsonUnmarshal(Data);
    } else {
        return false;
    }

    return true;
}