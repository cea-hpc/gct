/*
 * This file is licensed under the terms of the Globus Toolkit Public
 * License, found at http://www.globus.org/toolkit/download/license.html.
 */
package org.globus.ogsa.impl.base.multirft;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;

import java.net.MalformedURLException;
import java.net.URL;

import java.rmi.RemoteException;

import java.util.HashMap;
import java.util.Map;
import java.util.Vector;

import javax.security.auth.Subject;

import javax.xml.namespace.QName;

import org.apache.axis.MessageContext;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.globus.axis.gsi.GSIConstants;

import org.globus.gsi.jaas.JaasGssUtil;

import org.globus.ogsa.GridConstants;
import org.globus.ogsa.GridContext;
import org.globus.ogsa.GridServiceException;
import org.globus.ogsa.ServiceData;
import org.globus.ogsa.ServiceProperties;
import org.globus.ogsa.base.multirft.FileTransferJobStatusType;

import org.globus.ogsa.base.multirft.FileTransferProgressType;
import org.globus.ogsa.base.multirft.FileTransferRestartMarker;
import org.globus.ogsa.base.multirft.FileTransferStatusElement;
import org.globus.ogsa.base.multirft.GridFTPPerfMarkerElement;
import org.globus.ogsa.base.multirft.GridFTPPerfMarkerType;
import org.globus.ogsa.base.multirft.GridFTPRestartMarkerElement;
import org.globus.ogsa.base.multirft.GridFTPRestartMarkerType;
import org.globus.ogsa.base.multirft.RFTOptionsType;
import org.globus.ogsa.base.multirft.RFTPortType;
import org.globus.ogsa.base.multirft.TransferRequestElement;
import org.globus.ogsa.base.multirft.TransferRequestType;
import org.globus.ogsa.base.multirft.TransferStatusType;
import org.globus.ogsa.base.multirft.TransferType;
import org.globus.ogsa.base.multirft.Version;
import org.globus.ogsa.config.ConfigException;
import org.globus.ogsa.config.ContainerConfig;
import org.globus.ogsa.impl.base.multirft.MyMarkerListener;
import org.globus.ogsa.impl.base.multirft.TransferDbAdapter;
import org.globus.ogsa.impl.base.multirft.TransferDbOptions;
import org.globus.ogsa.impl.base.multirft.TransferJob;
import org.globus.ogsa.impl.base.multirft.util.URLExpander;
import org.globus.ogsa.impl.core.handle.HandleHelper;
import org.globus.ogsa.impl.ogsi.GridServiceImpl;
import org.globus.ogsa.impl.security.SecurityManager;
import org.globus.ogsa.impl.security.authentication.Constants;
import org.globus.ogsa.impl.security.authentication.SecContext;
import org.globus.ogsa.impl.security.authentication.SecureServicePropertiesHelper;
import org.globus.ogsa.impl.security.authorization.SelfAuthorization;
import org.globus.ogsa.utils.AnyHelper;
import org.globus.ogsa.utils.MessageUtils;
import org.globus.ogsa.wsdl.GSR;

import org.globus.util.GlobusURL;
import org.globus.util.Util;

import org.gridforum.ogsi.ServiceDataType;

import org.ietf.jgss.GSSCredential;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

/**
 *  Description of the Class
 *
 *@author     madduri
 *@created    September 17, 2003
 */
public class RftImpl
         extends GridServiceImpl {

    static Log logger = LogFactory.getLog(RftImpl.class.getName());
    boolean connectionPoolingEnabled = false; //no connection pooling
    String configPath;
    TransferRequestType transferRequest;
    TransferRequestElement transferRequestElement;
    TransferType[] transfers;
    private Map notifyProps;
    int concurrency;
    int maxAttempts = 10;
    TransferDbAdapter dbAdapter;
    TransferDbOptions dbOptions;
    ServiceData transferProgressData;
    ServiceData restartMarkerServiceData;
    ServiceData requestStatusData;
    ServiceData singleFileTransferStatusSDE;
    ServiceData gridFTPRestartMarkerSD;
    ServiceData gridFTPPerfMarkerSD;
    ServiceData versionSD;
    FileTransferProgressType transferProgress;
    FileTransferRestartMarker restartMarkerDataType;
    FileTransferStatusElement[] fileTransferStatusElements;
    FileTransferStatusElement fileTransferStatusElement;
    FileTransferJobStatusType[] statusTypes;
    GridFTPRestartMarkerElement gridFTPRestartMarkerSDE;
    GridFTPPerfMarkerElement gridFTPPerfMarkerSDE;
    Version version;
    int requestId = -1;
    private int persistentRequestId = 0;
    private int requestId_ = 0;
    private int transferJobId_ = 0;
    private boolean check = false;
    // check to update transferids of Status SDEs
    Vector activeTransferThreads;
    Vector transferClients;
    RFTOptionsType globalRFTOptionsType;
    private String proxyLocation = null;


    /**
     *  Constructor for the RftImpl object
     */
    public RftImpl() {
        super( "MultifileRFTService" );
        this.transferRequest = null;
    }


    /**
     *  Constructor for the RftImpl object
     *
     *@param  transferRequest  Description of the Parameter
     */
    public RftImpl( TransferRequestType transferRequest ) {
        super( "MultifileRFTService" );

        String name = "MultifileRFTService";

        this.transferRequest = transferRequest;
        this.globalRFTOptionsType = transferRequest.getRftOptions();

        if ( transferRequest == null ) {
            logger.debug( "transfer request is null" );
        }
    }


    /**
     *  DOCUMENT ME!
     *
     *@return                   requestId
     *@throws  RemoteException
     */
    public int start()
             throws RemoteException {

        Subject subject = SecurityManager.getManager().setServiceOwnerFromContext(
                this );
        GSSCredential cred = JaasGssUtil.getCredential( subject );

        if ( cred == null ) {
            throw new RemoteException( "Delegation not performed" );
        }

        try {

            String path = TransferClient.saveCredential( cred );
            Util.setFilePermissions( path, 600 );
            dbAdapter.storeProxyLocation( requestId, path );
            this.proxyLocation = path;
            int transferCount = dbAdapter.getTransferCount();

            int temp = 0;

            while ( temp < concurrency ) {

                TransferJob transferJob = new TransferJob( transfers[temp],
                        TransferJob.STATUS_PENDING,
                        0 );
                processURLs( transferJob );
                transferJob.setTransferId(transferCount);
                dbAdapter.update(transferJob);
                TransferThread transferThread = new TransferThread( transferJob );
                transferThread.start();
                activeTransferThreads.add( transferThread );
                temp = temp + 1;
                transferCount = transferCount + 1;
            }
        } catch ( Exception e ) {
            logger.error( "Error in start " + e.toString(), e );
            throw new RemoteException( MessageUtils.toString( e ) );
        }

        return requestId;
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  requestId         DOCUMENT ME!
     *@param  fromId            DOCUMENT ME!
     *@param  toId              DOCUMENT ME!
     *@throws  RemoteException  DOCUMENT ME!
     */
    public void cancel( int requestId, int fromId, int toId )
             throws RemoteException {
        logger.debug( "Cancelling transfers of the request: " + requestId );
        logger.debug( "from id: " + fromId + "to id: " + toId );
        dbAdapter.cancelTransfers( requestId, fromId, toId );
        cancelActiveTransfers( fromId, toId );
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  fromId           DOCUMENT ME!
     *@param  toId             DOCUMENT ME!
     *@throws  RftDBException  DOCUMENT ME!
     */
    public void cancelActiveTransfers( int fromId, int toId )
             throws RftDBException {

        for ( int i = fromId; i <= toId; i++ ) {

            TransferThread tempTransferThread = (TransferThread) activeTransferThreads.elementAt(
                    i );
            tempTransferThread.killThread();
        }
    }


    /**
     *  Description of the Method
     *
     *@param  transferJob  Description of the Parameter
     *@return              Description of the Return Value
     */
    public TransferJob processURLs( TransferJob transferJob ) {
        logger.debug( "checking to see if destination URL is a directory" );
        String destinationURL = transferJob.getDestinationUrl();
        String sourceURL = transferJob.getSourceUrl();

        if ( ( destinationURL.endsWith( "/" ) ) && !( sourceURL.endsWith( "/" ) ) ) {
            logger.debug( "The destinationURL : " + destinationURL +
                    " appears to be a directory" );
            String fileName = extractFileName( sourceURL );
            destinationURL = destinationURL + fileName;
            transferJob.setDestinationUrl( destinationURL );
            try {
                dbAdapter.update( transferJob );
            } catch ( RftDBException rdb ) {
                logger.debug( "Error processing urls" );
            }
            //change the destUrl by appending filename to it
        }
        return transferJob;
    }

    /*
     *  Description of the Method
     *
     *@param  sourceURL  Description of the Parameter
     *@return            Description of the Return Value
     */
      public String extractFileName( String sourceURL ) {
        return sourceURL.substring( sourceURL.lastIndexOf( "/" ) + 1 );
    }
        
    /**
     *  DOCUMENT ME!
     *
     *@param  messageContext         DOCUMENT ME!
     *@throws  GridServiceException  DOCUMENT ME!
     */
    public void postCreate( GridContext messageContext )
             throws GridServiceException {
        try {
            super.postCreate( messageContext );
            ServiceProperties factoryProperties = (ServiceProperties) getProperty(
                    ServiceProperties.FACTORY );
            //turn on connection pooling if requested
            String connectionPoolingValue
                = (String) factoryProperties.getProperty("connection.pooling");
            if( (connectionPoolingValue != null) 
                && (connectionPoolingValue.equalsIgnoreCase("true"))) {
                this.connectionPoolingEnabled = true;
            }
            transferProgress = new FileTransferProgressType();
            restartMarkerDataType = new FileTransferRestartMarker();
            fileTransferStatusElement = new FileTransferStatusElement();
            gridFTPRestartMarkerSDE = new GridFTPRestartMarkerElement();
            gridFTPPerfMarkerSDE = new GridFTPPerfMarkerElement();
            version = new Version();
            this.requestStatusData = this.serviceData.create(
                new QName("FileTransferStatus"));
            this.transferProgressData = this.serviceData.create(
                new QName("FileTransferProgress"));
            this.singleFileTransferStatusSDE = this.serviceData.create(
                new QName("SingleFileTransferStatus"));
            this.restartMarkerServiceData = this.serviceData.create(
                new QName("FileTransferRestartMarker"));
            this.gridFTPRestartMarkerSD = this.serviceData.create(
                new QName("GridFTPRestartMarker"));
            this.gridFTPPerfMarkerSD = this.serviceData.create(
                new QName("GridFTPPerfMarker"));
            this.versionSD = this.serviceData.create(
                new QName("MultiRFTVersion"));
            this.version.setVersion("1.0");
            this.versionSD.setValue(this.version);
            this.serviceData.add(this.versionSD);
            int progressInt = 0;
            this.transferProgress.setPercentComplete( progressInt );
            this.transferProgressData.setValue( transferProgress );
            this.serviceData.add( transferProgressData );
            this.restartMarkerDataType.setRestartMarkerRange( progressInt );
            this.restartMarkerServiceData.setValue( restartMarkerDataType );
            this.serviceData.add( restartMarkerServiceData );

            FileTransferJobStatusType statusType = new FileTransferJobStatusType();
            statusType.setTransferId( -1 );
            statusType.setDestinationUrl( "destURLPlaceHolder" );
            statusType.setStatus( null );
            fileTransferStatusElement.setRequestStatus( statusType );
            this.singleFileTransferStatusSDE.setValue(
                    fileTransferStatusElement );
            this.serviceData.add( singleFileTransferStatusSDE );
            gridFTPRestartMarkerSDE.setGridFTPRestartMarker( new GridFTPRestartMarkerType() );
            this.gridFTPRestartMarkerSD.setValue( this.gridFTPRestartMarkerSDE );
            this.serviceData.add( this.gridFTPRestartMarkerSD );
            gridFTPPerfMarkerSDE.setGridFTPPerfMarker( new GridFTPPerfMarkerType() );
            this.gridFTPPerfMarkerSD.setValue( this.gridFTPPerfMarkerSDE );
            this.serviceData.add( this.gridFTPPerfMarkerSD );

            String persistentRequestIdString = (String) getPersistentProperty(
                    "requestId" );
            String temp = (String) factoryProperties.getProperty( "maxAttempts" );
            int maxAttempts = Integer.parseInt( temp );
            String jdbcDriver = (String) factoryProperties.getProperty(
                    "JdbcDriver" );
            String connectionURL = (String) factoryProperties.getProperty(
                    "connectionURL" );
            String userName = (String) factoryProperties.getProperty(
                    "dbusername" );
            String password = (String) factoryProperties.getProperty( "password" );
            dbOptions = new TransferDbOptions( jdbcDriver, connectionURL,
                    userName, password );
            dbAdapter = TransferDbAdapter.setupDBConnection( dbOptions );
            activeTransferThreads = new Vector();
            transferClients = new Vector();

            if ( persistentRequestIdString != null ) {
                logger.debug(
                        "recovering transfer request: " +
                        persistentRequestIdString );
                this.persistentRequestId = Integer.parseInt(
                        persistentRequestIdString );
                this.requestId = this.persistentRequestId;

                String proxyLocation = dbAdapter.getProxyLocation(
                        this.persistentRequestId );
                this.proxyLocation = proxyLocation;
                GSSCredential credential = TransferClient.loadCredential(
                        proxyLocation );
                setNotifyProps( credential, Constants.ENCRYPTION );

                Vector recoveredTransferJobs = dbAdapter.getActiveTransfers(
                        persistentRequestId );
                int tempSize = recoveredTransferJobs.size();
                transfers = new TransferType[tempSize];

                for ( int i = 0; i < tempSize; i++ ) {

                    TransferJob transferJob = (TransferJob) recoveredTransferJobs.elementAt(
                            i );

                    //converting recovered transfers to transfer types
                    transfers[i] = new TransferType();
                    // transfers[i].setTransferId(transferJob.getTransferId());
                    transfers[i].setSourceUrl( transferJob.getSourceUrl() );
                    transfers[i].setDestinationUrl( transferJob.getDestinationUrl() );
                    transfers[i].setRftOptions( transferJob.getRftOptions() );
                }

                int concurrency_ = dbAdapter.getConcurrency(
                        this.persistentRequestId );
                logger.debug(
                        "Concurrency of recovered request: " + concurrency_ );
                logger.debug( "Populating FileTransferStatus SDEs" );
                fileTransferStatusElements = new FileTransferStatusElement[transfers.length];
                statusTypes = new FileTransferJobStatusType[transfers.length];

                // transferJobId_ = dbAdapter.getTransferJobId(requestId);
                for ( int i = 0; i < transfers.length; i++ ) {
                    statusTypes[i] = new FileTransferJobStatusType();
                    statusTypes[i].setTransferId( transfers[i].getTransferId() );
                    statusTypes[i].setDestinationUrl( transfers[i].getDestinationUrl() );
                    statusTypes[i].setStatus( mapStatus( TransferJob.STATUS_PENDING ) );
                    fileTransferStatusElements[i] = new FileTransferStatusElement();
                    fileTransferStatusElements[i].setRequestStatus(
                            statusTypes[i] );
                }

                requestStatusData.setValues(
                        new Object[]{fileTransferStatusElements} );
                this.serviceData.add( requestStatusData );

                for ( int i = 0; i < concurrency_; i++ ) {

                    TransferJob transferJob = (TransferJob) recoveredTransferJobs.elementAt(
                            i );
                    int tempStatus = transferJob.getStatus();

                    if ( ( tempStatus == TransferJob.STATUS_ACTIVE ) ||
                            ( tempStatus == TransferJob.STATUS_PENDING ) ||
                            ( tempStatus == TransferJob.STATUS_PENDING ) ) {

                        TransferThread transferThread = new TransferThread(
                                transferJob );
                        logger.debug( "Starting recovered transfer jobs " );
                        transferThread.start();
                    }
                }
            } else {
                SecurityManager manager = SecurityManager.getManager();

                GSSCredential cred = SecureServicePropertiesHelper.getCredential(
                        this );
                transfers = this.transferRequest.getTransferArray();
                this.concurrency = transferRequest.getConcurrency();
                requestId = dbAdapter.storeTransferRequest(
                        this.transferRequest );
                setPersistentProperty( "requestId", Integer.toString( requestId ) );
                setPersistentProperty( "activateOnStartup",
                        Boolean.TRUE.toString() );
                flush();
                this.persistentRequestId = requestId;
                logger.debug( "Populating FileTransferStatus SDEs" );
                fileTransferStatusElements = new FileTransferStatusElement[transfers.length];
                statusTypes = new FileTransferJobStatusType[transfers.length];
                transferJobId_ = dbAdapter.getTransferJobId( requestId );
                logger.debug(
                        "setting transferid in statusTypes to : " +
                        transferJobId_ );

                for ( int i = 0; i < transfers.length; i++ ) {
                    statusTypes[i] = new FileTransferJobStatusType();
                    statusTypes[i].setTransferId( transferJobId_++ );
                    statusTypes[i].setDestinationUrl( transfers[i].getDestinationUrl() );
                    statusTypes[i].setStatus( mapStatus( TransferJob.STATUS_PENDING ) );
                    fileTransferStatusElements[i] = new FileTransferStatusElement();
                    fileTransferStatusElements[i].setRequestStatus(
                            statusTypes[i] );
                }

                requestStatusData.setValues(
                        new Object[]{fileTransferStatusElements} );
                this.serviceData.add( requestStatusData );
            }
        } catch ( Exception e ) {
            throw new GridServiceException( e );
        }
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  transferStatus  DOCUMENT ME!
     *@return                 DOCUMENT ME!
     */
    private TransferStatusType mapStatus( int transferStatus ) {

        if ( transferStatus == 0 ) {

            return TransferStatusType.Finished;
        }

        if ( transferStatus == 1 ) {

            return TransferStatusType.Retrying;
        }

        if ( transferStatus == 2 ) {

            return TransferStatusType.Failed;
        }

        if ( transferStatus == 3 ) {

            return TransferStatusType.Active;
        }

        if ( transferStatus == 4 ) {

            return TransferStatusType.Pending;
        }

        if ( transferStatus == 5 ) {

            return TransferStatusType.Cancelled;
        }

        return null;
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  credential  DOCUMENT ME!
     *@param  msgProt     DOCUMENT ME!
     */
    private void setNotifyProps( GSSCredential credential, Object msgProt ) {
        this.notifyProps = new HashMap();
        this.notifyProps.put( GSIConstants.GSI_MODE,
                GSIConstants.GSI_MODE_NO_DELEG );
        this.notifyProps.put( Constants.GSI_SEC_CONV, msgProt );
        this.notifyProps.put( Constants.AUTHORIZATION,
                SelfAuthorization.getInstance() );
        this.notifyProps.put( GSIConstants.GSI_CREDENTIALS, credential );
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  context                   Description of the Parameter
     *@exception  GridServiceException  Description of the Exception
     *@throws  Exception                DOCUMENT ME!
     */
    public void preDestroy( GridContext context )
             throws GridServiceException {
        super.preDestroy( context );
        logger.debug("Removing the delegated proxy from : " + this.proxyLocation);
        Util.destroy(this.proxyLocation);
        logger.debug( "RFT instance destroyed" );
    }


    /**
     *  DOCUMENT ME!
     *
     *@param  transferJob            DOCUMENT ME!
     *@throws  GridServiceException  DOCUMENT ME!
     */
    public void statusChanged( TransferJob transferJob )
             throws GridServiceException {
        logger.debug( "Single File Transfer Status SDE changed " + transferJob.getStatus() );
        dbAdapter.update( transferJob );
        transferJobId_ = transferJob.getTransferId();

        FileTransferJobStatusType statusType = new FileTransferJobStatusType();
        statusType.setTransferId( transferJob.getTransferId() );
        statusType.setDestinationUrl( transferJob.getDestinationUrl() );
        statusType.setStatus( mapStatus( transferJob.getStatus() ) );
        this.fileTransferStatusElement.setRequestStatus( statusType );
        this.singleFileTransferStatusSDE.setValue( fileTransferStatusElement );
        this.serviceData.add( singleFileTransferStatusSDE );
        singleFileTransferStatusSDE.notifyChange();

        for ( int i = 0; i < transfers.length; i++ ) {

            if ( statusTypes[i].getTransferId() == transferJob.getTransferId() ) {
                statusTypes[i].setStatus( mapStatus( transferJob.getStatus() ) );
            }
        }
    }


    /**
     *  Gets the transferClient attribute of the RftImpl object
     *
     *@param  sourceURL                  Description of the Parameter
     *@param  destinationURL             Description of the Parameter
     *@return                            The transferClient value
     *@exception  MalformedURLException  Description of the Exception
     */
    public TransferClient getTransferClient( String sourceURL, String destinationURL )
             throws MalformedURLException {
        TransferClient transferClient = null;
        boolean flag = false;
        for ( int i = 0; i < this.transferClients.size(); i++ ) {
            TransferClient tempTransferClient = (TransferClient) this.transferClients.elementAt( i );
            GlobusURL source = tempTransferClient.getSourceURL();
            GlobusURL destination = tempTransferClient.getDestinationURL();
            int status = tempTransferClient.getStatus();
            GlobusURL tempSource = new GlobusURL( sourceURL );
            GlobusURL tempDest = new GlobusURL( destinationURL );
            if ( status != 3 ) {
                flag = true;
            }
            if ( ( source.getHost().equals( tempSource.getHost() ) ) && ( destination.getHost().equals( tempDest.getHost() ) ) && flag ) {
                transferClient = tempTransferClient;
                transferClient.setSourcePath( tempSource.getPath() );
                transferClient.setDestinationPath( tempDest.getPath() );
                logger.debug( "status: " + status );
                return transferClient;
            }
        }
        return transferClient;
    }


    /**
     *  Description of the Class
     *
     *@author     madduri
     *@created    September 17, 2003
     */
    public class TransferThread
             extends Thread {

        TransferJob transferJob;
        TransferClient transferClient;
        int status;
        int attempts;
        BufferedReader stdInput;
        BufferedReader stdError;


        /**
         *  Constructor for the TransferThread object
         *
         *@param  transferJob  Description of the Parameter
         */
        TransferThread( TransferJob transferJob ) {
            this.transferJob = transferJob;
            this.attempts = transferJob.getAttempts();
            this.status = transferJob.getStatus();
        }


        /**
         *  Sets the transferClient attribute of the TransferThread object
         *
         *@param  transferClient  The new transferClient value
         */
        public void setTransferClient( TransferClient transferClient ) {
            this.transferClient = transferClient;
        }


        /**
         *  DOCUMENT ME!
         *
         *@throws  RftDBException  DOCUMENT ME!
         */
        public void killThread()
                 throws RftDBException {
            transferJob.setStatus( TransferJob.STATUS_CANCELLED );
            dbAdapter.update( transferJob );
        }


        /**
         *  DOCUMENT ME!
         */
        public void run() {

            try {

                int tempId = transferJob.getTransferId();
                TransferThread transferThread;
                RFTOptionsType rftOptions = transferJob.getRftOptions();
                if ( rftOptions == null ) {
                    rftOptions = globalRFTOptionsType;
                }
                try {
                    transferClient = getTransferClient( transferJob.getSourceUrl(),
                            transferJob.getDestinationUrl() );

                    if ( (transferClient == null ) 
                        || !connectionPoolingEnabled) {
                        logger.debug( "No transferClient reuse" + proxyLocation );
                        transferClient = new TransferClient( tempId,
                                transferJob.getSourceUrl(),
                                transferJob.getDestinationUrl(),
                                proxyLocation,
                                transferProgress,
                                serviceData,
                                transferProgressData,
                                restartMarkerServiceData,
                                restartMarkerDataType,
                                gridFTPRestartMarkerSD,
                                gridFTPRestartMarkerSDE,
                                gridFTPPerfMarkerSD,
                                gridFTPPerfMarkerSDE,
                                rftOptions );
                        transferJob.setStatus( TransferJob.STATUS_ACTIVE );
                        dbAdapter.update( transferJob );
                    } else {
                        logger.debug( "Reusing TransferClient from the pool" );
                        transferClient.setSourceURL( transferJob.getSourceUrl() );
                        transferClient.setDestinationURL( transferJob.getDestinationUrl() );
                       
                         MyMarkerListener myMarkerListener = new
                         MyMarkerListener( transferProgress, serviceData
                         , transferProgressData, transferClient.getSize()
                         , restartMarkerServiceData, restartMarkerDataType
                         , gridFTPRestartMarkerSD, gridFTPRestartMarkerSDE
                         , gridFTPPerfMarkerSD, gridFTPPerfMarkerSDE );
                         transferClient.setMyMarkerListener( myMarkerListener );
                        transferClient.setStatus( TransferJob.STATUS_ACTIVE );
                    }
                } catch ( Exception e ) {
                    logger.error( "Error in Transfer Client" + e.toString(), e );
                    transferJob.setStatus( TransferJob.STATUS_FAILED );
                    statusChanged( transferJob );

                    TransferJob newTransferJob = dbAdapter.getTransferJob(
                            requestId );

                    if ( newTransferJob != null ) {
                        transferThread = new TransferThread( newTransferJob );
                        logger.debug(
                                "Attempts in new transfer: " +
                                newTransferJob.getAttempts() );
                        transferThread.start();
                        newTransferJob.setStatus( TransferJob.STATUS_ACTIVE );
                        statusChanged( newTransferJob );
                    } else {
                        logger.debug( "No more transfers " );
                    }

                    throw new RemoteException( MessageUtils.toString( e ) );
                }

                String restartMarker = dbAdapter.getRestartMarker( tempId );
                boolean useExtended = false;

                if ( restartMarker != null ) {
                    transferClient.setRestartMarker( restartMarker );
                    useExtended = true;

                }
                if ( transferClient != null ) {
                    if ( transferClient.getStatus() == 6 ) {
                        transferClient.setStatus( TransferJob.STATUS_EXPANDING );
                        transferJob.setStatus( TransferJob.STATUS_EXPANDING );
                    } else {
                        transferClient.setStatus( TransferJob.STATUS_ACTIVE );
                        transferClient.setParallelStreams( rftOptions.getParallelStreams() );
                        transferClient.setTcpBufferSize( rftOptions.getTcpBufferSize() );
                        transferClient.setRFTOptions( rftOptions );
                        transferClient.transfer(useExtended);
                        transferJob.setStatus( TransferJob.STATUS_ACTIVE );
                        dbAdapter.update( transferJob );
                        transferJob.setStatus( transferClient.getStatus() );
                        statusChanged( transferJob );

                        int x = transferClient.getStatus();
                        transferJob.setAttempts( transferJob.getAttempts() + 1 );

                        if ( x == 0 ) {
                            transferJob.setStatus( TransferJob.STATUS_FINISHED );
                            this.status = TransferJob.STATUS_FINISHED;
                            logger.debug( "Transfer " + transferJob.getTransferId() + " DONE" );
                            statusChanged( transferJob );
                            transferProgress.setPercentComplete( 100 );
                            transferProgressData.setValue( transferProgress );
                            transferClient.setStatus( TransferJob.STATUS_FINISHED );
                            transferClients.add( transferClient );
                        } else if ( ( x == 1 ) &&
                                ( transferJob.getAttempts() < maxAttempts ) ) {
                            transferJob.setStatus( TransferJob.STATUS_PENDING );
                            transferClient.setStatus( TransferJob.STATUS_PENDING );
                            this.status = TransferJob.STATUS_PENDING;
                            logger.debug( "Transfer " + transferJob.getTransferId() + " Retrying" );
                            statusChanged( transferJob );

                        } else if ( ( x == 2 ) ||
                                ( transferJob.getAttempts() >= maxAttempts ) ) {
                            transferJob.setStatus( TransferJob.STATUS_FAILED );
                            this.status = TransferJob.STATUS_FAILED;
                            logger.debug( "Transfer " + transferJob.getTransferId() + " Failed" );
                            statusChanged( transferJob );
                            transferClient.setStatus( TransferJob.STATUS_FAILED );
                            transferClients.add( transferClient );
                        } else {
                            transferJob.setStatus( TransferJob.STATUS_RETRYING );
                            transferClient.setStatus( TransferJob.STATUS_RETRYING );
                            this.status = TransferJob.STATUS_RETRYING;
                            statusChanged( transferJob );
                        }
                    }
                } else {
                    transferJob.setStatus( TransferJob.STATUS_FAILED );
                    this.status = TransferJob.STATUS_FAILED;
                    statusChanged( transferJob );
                    transferClient.setStatus( TransferJob.STATUS_FAILED );
                    transferClients.add( transferClient );
                }

                dbAdapter.update( transferJob );

                TransferJob newTransferJob = dbAdapter.getTransferJob(
                        requestId );

                if ( newTransferJob != null ) {
                    logger.debug( "starting a new transfer " + newTransferJob.getTransferId() + "  " + newTransferJob.getStatus() );
                    transferThread = new TransferThread( newTransferJob );
                    transferThread.start();
                    newTransferJob.setStatus( TransferJob.STATUS_ACTIVE );
                    statusChanged( newTransferJob );
                } else {
                    URLExpander urlExpander = transferClient.getUrlExpander();
                    if ( urlExpander != null ) {
                        boolean expStatus = urlExpander.getStatus();
                        while ( expStatus == false ) {
                            try {
                                logger.debug( "Sleeping Here" );
                                //FIX THIS
                                sleep( 2000 );
                                expStatus = urlExpander.getStatus();
                            } catch ( InterruptedException ie ) {
                            }
                        }
						int tempConc = 0;
                        
                        TransferJob newTransferJob1 = dbAdapter.getTransferJob(requestId);
                    	if ( newTransferJob1 == null ) {
                            	logger.debug( "No more transfers " );
                        	} else {
                            	transferThread = new TransferThread( newTransferJob1 );
                            	transferThread.start();
                            	newTransferJob1.setStatus( TransferJob.STATUS_ACTIVE );
                            	statusChanged( newTransferJob1 );
                        	}

					}
                }
            } catch ( Exception ioe ) {
                logger.error( "Error in Transfer Thread" + ioe.toString(), ioe );
            } catch ( Throwable ee ) {
                logger.error( "Error in Transfer Thread" + ee.toString(), ee );
            }
        }
    }
}

