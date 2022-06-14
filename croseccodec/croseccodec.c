#define DESCRIPTOR_DEF
#include "driver.h"
#include "stdint.h"

#define bool int
#define MS_IN_US 1000

static ULONG CrosEcCodecDebugLevel = 100;
static ULONG CrosEcCodecDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS
DriverEntry(
__in PDRIVER_OBJECT  DriverObject,
__in PUNICODE_STRING RegistryPath
)
{
	NTSTATUS               status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	CrosEcCodecPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"Driver Entry\n");

	WDF_DRIVER_CONFIG_INIT(&config, CrosEcCodecEvtDeviceAdd);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	//
	// Create a framework driver object to represent our driver.
	//

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		WDF_NO_HANDLE
		);

	if (!NT_SUCCESS(status))
	{
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
			"WdfDriverCreate failed with status 0x%x\n", status);
	}

	return status;
}

NTSTATUS ConnectToEc(
	_In_ WDFDEVICE FxDevice
) {
	PCROSECCODEC_CONTEXT pDevice = GetDeviceContext(FxDevice);
	WDF_OBJECT_ATTRIBUTES objectAttributes;

	WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
	objectAttributes.ParentObject = FxDevice;

	NTSTATUS status = WdfIoTargetCreate(FxDevice,
		&objectAttributes,
		&pDevice->busIoTarget
	);
	if (!NT_SUCCESS(status))
	{
		CrosEcCodecPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error creating IoTarget object - 0x%x\n",
			status);
		if (pDevice->busIoTarget)
			WdfObjectDelete(pDevice->busIoTarget);
		return status;
	}

	DECLARE_CONST_UNICODE_STRING(busDosDeviceName, L"\\DosDevices\\GOOG0004");

	WDF_IO_TARGET_OPEN_PARAMS openParams;
	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
		&openParams,
		&busDosDeviceName,
		(GENERIC_READ | GENERIC_WRITE));

	openParams.ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
	openParams.CreateDisposition = FILE_OPEN;
	openParams.FileAttributes = FILE_ATTRIBUTE_NORMAL;

	CROSEC_INTERFACE_STANDARD CrosEcInterface;
	RtlZeroMemory(&CrosEcInterface, sizeof(CrosEcInterface));

	status = WdfIoTargetOpen(pDevice->busIoTarget, &openParams);
	if (!NT_SUCCESS(status))
	{
		CrosEcCodecPrint(
			DEBUG_LEVEL_ERROR,
			DBG_IOCTL,
			"Error opening IoTarget object - 0x%x\n",
			status);
		WdfObjectDelete(pDevice->busIoTarget);
		return status;
	}

	status = WdfIoTargetQueryForInterface(pDevice->busIoTarget,
		&GUID_CROSEC_INTERFACE_STANDARD,
		(PINTERFACE)&CrosEcInterface,
		sizeof(CrosEcInterface),
		1,
		NULL);
	WdfIoTargetClose(pDevice->busIoTarget);
	pDevice->busIoTarget = NULL;
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfFdoQueryForInterface failed 0x%x\n", status);
		return status;
	}

	pDevice->CrosEcBusContext = CrosEcInterface.InterfaceHeader.Context;
	pDevice->CrosEcCmdXferStatus = CrosEcInterface.CmdXferStatus;
	return status;
}

NTSTATUS
OnPrepareHardware(
_In_  WDFDEVICE     FxDevice,
_In_  WDFCMRESLIST  FxResourcesRaw,
_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

This routine caches the SPB resource connection ID.

Arguments:

FxDevice - a handle to the framework device object
FxResourcesRaw - list of translated hardware resources that
the PnP manager has assigned to the device
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	PCROSECCODEC_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

	UNREFERENCED_PARAMETER(FxResourcesRaw);
	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	status = ConnectToEc(FxDevice);
	if (!NT_SUCCESS(status)) {
		return status;
	}


	(*pDevice->CrosEcCmdXferStatus)(pDevice->CrosEcBusContext, NULL);

	return status;
}

NTSTATUS
OnReleaseHardware(
_In_  WDFDEVICE     FxDevice,
_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

Arguments:

FxDevice - a handle to the framework device object
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	return status;
}

static NTSTATUS send_ec_command(
	_In_ PCROSECCODEC_CONTEXT pDevice,
	UINT32 cmd,
	UINT8* out,
	size_t outSize,
	UINT8* in,
	size_t inSize)
{
	struct cros_ec_command* msg = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct cros_ec_command) + max(outSize, inSize), CROSECCODEC_POOL_TAG);
	if (!msg) {
		return STATUS_NO_MEMORY;
	}
	msg->version = 0;
	msg->command = cmd;
	msg->outsize = outSize;
	msg->insize = inSize;

	if (outSize)
		memcpy(msg->data, out, outSize);

	NTSTATUS status = (*pDevice->CrosEcCmdXferStatus)(pDevice->CrosEcBusContext, msg);
	if (!NT_SUCCESS(status)) {
		goto exit;
	}

	if (in && inSize) {
		memcpy(in, msg->data, inSize);
	}

exit:
	ExFreePoolWithTag(msg, CROSECCODEC_POOL_TAG);
	return status;
}

VOID
CrosEcBootWorkItem(
	IN WDFWORKITEM  WorkItem
)
{
	WDFDEVICE Device = (WDFDEVICE)WdfWorkItemGetParentObject(WorkItem);
	PCROSECCODEC_CONTEXT pDevice = GetDeviceContext(Device);

	struct ec_param_ec_codec_i2s_rx i2sRxCmd;

	RtlZeroMemory(&i2sRxCmd, sizeof(i2sRxCmd));
	i2sRxCmd.cmd = EC_CODEC_I2S_RX_SET_SAMPLE_DEPTH;
	i2sRxCmd.set_sample_depth_param.depth = EC_CODEC_I2S_RX_SAMPLE_DEPTH_16;
	NTSTATUS status = send_ec_command(pDevice, EC_CMD_EC_CODEC_I2S_RX, (UINT8*)&i2sRxCmd, sizeof(i2sRxCmd), NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set set i2s sample depth\n");
		goto exit;
	}

	RtlZeroMemory(&i2sRxCmd, sizeof(i2sRxCmd));
	i2sRxCmd.cmd = EC_CODEC_I2S_RX_SET_BCLK;
	i2sRxCmd.set_bclk_param.bclk = 48000 * 64;
	status = send_ec_command(pDevice, EC_CMD_EC_CODEC_I2S_RX, (UINT8*)&i2sRxCmd, sizeof(i2sRxCmd), NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set set i2s bclk\n");
		goto exit;
	}

	RtlZeroMemory(&i2sRxCmd, sizeof(i2sRxCmd));
	i2sRxCmd.cmd = EC_CODEC_I2S_RX_SET_DAIFMT;
	i2sRxCmd.set_daifmt_param.daifmt = EC_CODEC_I2S_RX_DAIFMT_I2S;
	status = send_ec_command(pDevice, EC_CMD_EC_CODEC_I2S_RX, (UINT8*)&i2sRxCmd, sizeof(i2sRxCmd), NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set set i2s DAI fmt\n");
		goto exit;
	}

	RtlZeroMemory(&i2sRxCmd, sizeof(i2sRxCmd));
	i2sRxCmd.cmd = EC_CODEC_I2S_RX_ENABLE;
	status = send_ec_command(pDevice, EC_CMD_EC_CODEC_I2S_RX, (UINT8*)&i2sRxCmd, sizeof(i2sRxCmd), NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set set i2s enable\n");
		goto exit;
	}

exit:
	WdfObjectDelete(WorkItem);
}

void CrosEcCodecBootTimer(_In_ WDFTIMER hTimer) {
	WDFDEVICE Device = (WDFDEVICE)WdfTimerGetParentObject(hTimer);

	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_WORKITEM_CONFIG workitemConfig;
	WDFWORKITEM hWorkItem;

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, CROSECCODEC_CONTEXT);
	attributes.ParentObject = Device;
	WDF_WORKITEM_CONFIG_INIT(&workitemConfig, CrosEcBootWorkItem);

	WdfWorkItemCreate(&workitemConfig,
		&attributes,
		&hWorkItem);

	WdfWorkItemEnqueue(hWorkItem);
	WdfTimerStop(hTimer, FALSE);
}

NTSTATUS
OnD0Entry(
_In_  WDFDEVICE               FxDevice,
_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
)
/*++

Routine Description:

This routine allocates objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxPreviousState - previous power state

Return Value:

Status

--*/
{
	PCROSECCODEC_CONTEXT pDevice = GetDeviceContext(FxDevice);
	UNREFERENCED_PARAMETER(FxPreviousState);
	NTSTATUS status = STATUS_SUCCESS;

	struct ec_param_ec_codec_i2s_rx i2sRxCmd;
	struct ec_response_ec_codec_dmic_get_max_gain dmicGain;

	RtlZeroMemory(&i2sRxCmd, sizeof(i2sRxCmd));
	i2sRxCmd.cmd = EC_CODEC_I2S_RX_RESET;
	status = send_ec_command(pDevice, EC_CMD_EC_CODEC_I2S_RX, (UINT8*)&i2sRxCmd, sizeof(i2sRxCmd), NULL, 0);
	if (!NT_SUCCESS(status)) {
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set set i2s reset\n");
		return status;
	}

	{
		struct ec_param_ec_codec_dmic dmicCmd;
		RtlZeroMemory(&dmicCmd, sizeof(dmicCmd));
		dmicCmd.cmd = EC_CODEC_DMIC_GET_MAX_GAIN;

		status = send_ec_command(pDevice, EC_CMD_EC_CODEC_DMIC, (UINT8*)&dmicCmd, sizeof(dmicCmd), (UINT8*)&dmicGain, sizeof(dmicGain));
		if (!NT_SUCCESS(status)) {
			CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed to get max gain\n");
			return status;
		}

		RtlZeroMemory(&dmicCmd, sizeof(dmicCmd));
		dmicCmd.cmd = EC_CODEC_DMIC_SET_GAIN_IDX;
		dmicCmd.set_gain_idx_param.channel = EC_CODEC_DMIC_CHANNEL_0;
		dmicCmd.set_gain_idx_param.gain = dmicGain.max_gain;
		status = send_ec_command(pDevice, EC_CMD_EC_CODEC_DMIC, (UINT8*)&dmicCmd, sizeof(dmicCmd), NULL, 0);
		if (!NT_SUCCESS(status)) {
			CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set gain on channel 0\n");
			return status;
		}

		RtlZeroMemory(&dmicCmd, sizeof(dmicCmd));
		dmicCmd.cmd = EC_CODEC_DMIC_SET_GAIN_IDX;
		dmicCmd.set_gain_idx_param.channel = EC_CODEC_DMIC_CHANNEL_1;
		dmicCmd.set_gain_idx_param.gain = dmicGain.max_gain;
		status = send_ec_command(pDevice, EC_CMD_EC_CODEC_DMIC, (UINT8*)&dmicCmd, sizeof(dmicCmd), NULL, 0);
		if (!NT_SUCCESS(status)) {
			CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "Failed set gain on channel 1\n");
			return status;
		}
	}

	WDF_TIMER_CONFIG              timerConfig;
	WDFTIMER                      hTimer;
	WDF_OBJECT_ATTRIBUTES         attributes;

	WDF_TIMER_CONFIG_INIT(&timerConfig, CrosEcCodecBootTimer);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = pDevice->FxDevice;
	status = WdfTimerCreate(&timerConfig, &attributes, &hTimer);

	WdfTimerStart(hTimer, WDF_REL_TIMEOUT_IN_MS(2000));

	return status;
}

NTSTATUS
OnD0Exit(
_In_  WDFDEVICE               FxDevice,
_In_  WDF_POWER_DEVICE_STATE  FxTargetState
)
/*++

Routine Description:

This routine destroys objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxTargetState - target power state

Return Value:

Status

--*/
{
	UNREFERENCED_PARAMETER(FxTargetState);

	NTSTATUS status = STATUS_SUCCESS;

	return status;
}

NTSTATUS
CrosEcCodecEvtDeviceAdd(
IN WDFDRIVER       Driver,
IN PWDFDEVICE_INIT DeviceInit
)
{
	NTSTATUS                      status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDFDEVICE                     device;
	PCROSECCODEC_CONTEXT               devContext;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	CrosEcCodecPrint(DEBUG_LEVEL_INFO, DBG_PNP,
		"CrosEcCodecEvtDeviceAdd called\n");

	{
		WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
		WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

		pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
		pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
		pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
		pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;

		WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
	}

	//
	// Setup the device context
	//

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CROSECCODEC_CONTEXT);

	//
	// Create a framework device object.This call will in turn create
	// a WDM device object, attach to the lower stack, and set the
	// appropriate flags and attributes.
	//

	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

	if (!NT_SUCCESS(status))
	{
		CrosEcCodecPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceCreate failed with status code 0x%x\n", status);

		return status;
	}

	devContext = GetDeviceContext(device);

	devContext->FxDevice = device;

	return status;
}