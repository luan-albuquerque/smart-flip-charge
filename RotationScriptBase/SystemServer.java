
//adicione esses imports
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.view.Surface;

// DENTRO DO METODO startOthreServices() 


/*codigo implementado*/

        try {
    Slog.i(TAG, "Registering Custom Rotation Receiver");
    
    // Define o filtro de Intent para nosso gatilho
    IntentFilter filter = new IntentFilter("com.meu.device.ACTION_FORCE_ROTATE");
    
    // Pega o contexto do sistema
    Context systemContext = mSystemContext;

    // Cria e registra o receiver
    systemContext.registerReceiverAsUser(new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if ("com.meu.device.ACTION_FORCE_ROTATE".equals(intent.getAction())) {
                //valor padrao 0 caso vazio
                int rotation = intent.getIntExtra("rotation_angle", Surface.ROTATION_0);
                
                com.android.server.CustomRotationController.forceRotation(context, rotation);
            }
        }

}, UserHandle.ALL, filter, null, null, Context.RECEIVER_EXPORTED);
    
} catch (Exception e) {
    Slog.e(TAG, "Failed to register Custom Rotation Receiver", e);
}

/* fim do codigo*/
