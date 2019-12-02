<template>
  <div>
    <v-select
      v-if="responder"
      :options="chatOptions"
      v-model="destination"
      @select="onChatSelect"
      placeholder="select chat"
      label="label"
    >
      <span slot="no-options">{{
        responder ? 'no one needs assistance' : 'no responders found'
      }}</span>
    </v-select>
    <Chat
      :participants="participants"
      :myself="me"
      :messages="messages"
      :onType="onType"
      :onMessageSubmit="onMessageSubmit"
      :chatTitle="'Emergency Services'"
      :placeholder="'send message'"
      :colors="colors"
      :borderStyle="borderStyle"
      :hideCloseButton="true"
      :closeButtonIconSize="'20px'"
      :submitIconSize="'30px'"
      :asyncMode="false"
      class="chat"
    />
  </div>
</template>

<script>
import { mapMutations } from 'vuex'
import VueScrollTo from 'vue-scrollto'
import Chat from '~/components/chat/Chat.vue'
import { chatConf } from '~/assets/config'

const emergencyOptions = []
const emergencyServicesId = 2
const civilianId = 3

for (let i = 0; i < chatConf.emergencyServicesAddresses.length; i++) {
  emergencyOptions.push({
    label: `emergency services ${i + 1}`,
    value: chatConf.emergencyServicesAddresses[i]
  })
}

const messageType = '\x00'

export default {
  components: {
    Chat
  },
  data() {
    return {
      participants: [
        {
          name: 'Emergency Services',
          id: emergencyServicesId
        },
        {
          name: 'Civilian',
          id: civilianId
        }
      ],
      me: {
        name: 'me',
        id: 1
      },
      socket: null,
      responder: false,
      radiosource: '',
      websocketid: '',
      destination: null,
      cancleScroll: null,
      chatOptions: [],
      messages: [],
      colors: {
        header: {
          bg: '#2143cc',
          text: '#ffffff'
        },
        message: {
          myself: {
            bg: '#fff',
            text: '#0a0a0a'
          },
          others: {
            bg: '#4061e6',
            text: '#0a0a0a'
          },
          messagesDisplay: {
            bg: '#f7f3f3'
          }
        },
        submitIcon: '#b91010'
      },
      borderStyle: {
        topLeft: '10px',
        topRight: '10px',
        bottomLeft: '10px',
        bottomRight: '10px'
      }
    }
  },
  computed: {
    myself() {
      return this.$store.state.chat.myself
    }
  },
  beforeDestroy() {
    if (this.socket) this.socket.close()
    this.messages = []
  },
  mounted() {
    this.$axios
      .get(chatConf.configEndpoint)
      .then((res) => {
        if (res.status === 200 && res.data.hasOwnProperty('source')) {
          if (
            emergencyOptions.find(
              (option) =>
                option.value.charCodeAt(0) === parseInt(res.data.source)
            )
          ) {
            this.responder = true
            // the current node is the emergency worker
            this.updateChatSelection()
          } else {
            // the current node is a person who needs help
            this.destination = emergencyOptions[0]
          }
          this.socket = new window.WebSocket(
            `ws://${process.env.apiurl.replace('http://', '')}${
              chatConf.websocketEndpoint
            }`
          )
          this.radiosource = res.data.source
          this.socket.onopen = (evt) => {
            this.$toasted.global.success({
              message: 'Connection established'
            })
          }
          this.socket.onmessage = (evt) => {
            // console.log(`[message] Data received from server: ${evt.data}`)
            let jsonData
            try {
              jsonData = JSON.parse(evt.data)
            } catch (err) {
              this.$toasted.global.error({ message: JSON.stringify(err) })
            }
            if (jsonData.hasOwnProperty('debug')) {
              console.log(jsonData.debug)
            } else if (jsonData.hasOwnProperty('message')) {
              const message = {
                content: jsonData.message,
                myself: false,
                participantId: this.responder
                  ? emergencyServicesId
                  : civilianId,
                uploaded: false,
                viewed: false
              }
              this.newMessage(message)
              setTimeout(() => {
                this.scrollToBottom()
              }, 100)
            }
          }
          this.socket.onclose = (evt) => {
            if (evt.wasClean) {
              console.log(
                `[close] Connection closed cleanly, code=${evt.code} reason=${evt.reason}`
              )
            } else {
              this.$toasted.global.error({ message: '[close] Connection died' })
            }
          }
          this.socket.onerror = (err) => {
            console.error(`[error]: `, err)
          }
        } else {
          this.$toasted.global.warn({ message: 'no source found' })
        }
      })
      .catch((err) => {
        const message = 'got error with get config request'
        this.$toasted.global.error({ message })
        console.log(message, err)
      })
  },
  methods: {
    ...mapMutations({ newMessage: 'chat/newMessage' }),
    onChatSelect() {
      this.messages = []
    },
    updateChatSelection() {
      this.$axios
        .get(chatConf.senderListEndpoint)
        .then((res) => {
          if (res.status === 200 && res.data.hasOwnProperty('senders')) {
            const senders = []
            for (let i = 0; i < res.data.senders.length; i++) {
              senders.push({
                value: String.fromCharCode(res.data.senders[i]),
                label: `emergency ${i + 1}`
              })
            }
            this.chatOptions = senders
          } else {
            this.$toasted.global.warn({ message: 'no senders found' })
          }
        })
        .catch((err) => {
          this.$toasted.global.error({
            message: `got error with get senders request ${JSON.stringify(err)}`
          })
        })
    },
    onType(event) {
      // here you can set any behavior
    },
    scrollToBottom() {
      if (this.cancleScroll) {
        this.cancleScroll()
        this.cancleScroll = null
      }
      this.cancleScroll = VueScrollTo.scrollTo(
        `#message-${this.$store.state.chat.messages.length - 1}`,
        {
          container: '#containerMessageDisplay',
          offset: 0,
          force: true,
          cancelable: true
        }
      )
    },
    onMessageSubmit(message) {
      console.log(`New message sending: ${JSON.stringify(message)}`)
      if (this.destination) {
        const theMessage =
          this.destination.value + messageType + message.content
        this.socket.send(theMessage)
        message.uploaded = true
      } else {
        this.$toasted.global.error({ message: 'no destination found' })
      }
      setTimeout(() => {
        this.scrollToBottom()
      }, 100)
    }
  }
}
</script>

<style lang="scss" scoped>
.chat {
  max-width: 30rem;
  margin: auto;
  height: 40rem;
  @media only screen and (min-width: 600px) {
    margin-top: 2rem;
  }
}
.chat .custom-title {
  color: #ffffff;
  font-size: 20px;
}
</style>
